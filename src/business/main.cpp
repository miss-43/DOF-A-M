#include <sqlite3.h>
#include <stdio.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/face.hpp>
#include <opencv2/highgui.hpp>    // 解决 imshow, waitKey, destroyAllWindows
#include <opencv2/imgproc.hpp>    // 解决 cvtColor, equalizeHist, resize
#include <opencv2/objdetect.hpp>  // 包含 CascadeClassifier
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>  // 解决 VideoCapture, cap.read()
#include <thread>
#include <vector>

// 错误代码定义
enum ERROR_CODE {
  ERROR_SUCCESS = 0,
  ERROR_INIT_FAILED,
  ERROR_CAPTURE_FAILED,
  ERROR_TRAINING_FAILED,
  ERROR_RECOGNITION_FAILED,
  ERROR_MODEL_NOT_FOUND,
  ERROR_UNKNOWN
};

// 日志级别定义
enum LOG_LEVEL { LOG_DEBUG = 0, LOG_INFO, LOG_WARNING, LOG_ERROR };

// 全局配置常量
const int DEFAULT_USER_ID = 1;
const int SAMPLE_SIZE = 100;
const std::string MODEL_PATH = "./face_recognition_model.yml";
const std::string DATABASE_PATH = "./XL_URK_Database.db";

// 全局变量
cv::VideoCapture cap;
cv::CascadeClassifier face_cas;
cv::Ptr<cv::face::LBPHFaceRecognizer> recog;

// 数据结构
std::vector<cv::Mat> samples;
std::vector<int> labels;
std::vector<std::string> employee_names = {"user1", "user2", "user3", "user4",
                                           "user5"};

// 运行状态
bool is_recognizing = false;
int current_user_id = DEFAULT_USER_ID;
std::string current_user_name = "user1";
int sample_count = 0;

// 统一日志输出
void log_message(int code, const std::string& message) {
  std::string level_str;
  switch (code) {
    case ERROR_SUCCESS:
      level_str = "[INFO]";
      break;
    case ERROR_INIT_FAILED:
    case ERROR_CAPTURE_FAILED:
    case ERROR_TRAINING_FAILED:
    case ERROR_RECOGNITION_FAILED:
    case ERROR_MODEL_NOT_FOUND:
      level_str = "[ERROR]";
      break;
    default:
      level_str = "[UNKNOWN]";
      break;
  }
  std::cout << level_str << " " << message << std::endl;
}

// 初始化数据库连接
bool init_database() {
  sqlite3* db;
  sqlite3_stmt* stmt;
  int rc;

  // 打开数据库
  rc = sqlite3_open(DATABASE_PATH.c_str(), &db);
  if (rc) {
    log_message(ERROR_INIT_FAILED,
                "无法打开数据库: " + std::string(sqlite3_errmsg(db)));
    return false;
  }

  // 查询员工信息
  const char* sql_employee =
      "SELECT Employee_ID, Employee_Name FROM t_Employee";
  rc = sqlite3_prepare_v2(db, sql_employee, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    log_message(ERROR_INIT_FAILED,
                "查询员工信息失败: " + std::string(sqlite3_errmsg(db)));
    sqlite3_close(db);
    return false;
  }

  // 读取员工信息
  employee_names.clear();
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* emp_name = (const char*)sqlite3_column_text(stmt, 1);
    employee_names.push_back(std::string(emp_name));
  }
  sqlite3_finalize(stmt);
  sqlite3_close(db);

  log_message(ERROR_SUCCESS, "数据库初始化成功，加载了 " +
                                 std::to_string(employee_names.size()) +
                                 " 个员工信息");
  return true;
}

// 级联分类器，优先项目目录中的(修改成LBP)
static std::string find_cascade() {
  std::vector<std::string> paths = {
      "./lbpcascade_frontalface_improved.xml",
      "/usr/share/opencv/lbpcascades/lbpcascade_frontalface_improved.xml",
      "/usr/share/opencv4/lbpcascades/lbpcascade_frontalface_improved.xml"};
  for (auto& p : paths)
    if (std::filesystem::exists(p)) return p;
  return "";
}

// 返回图片中最大人脸
static cv::Rect detect_face(cv::Mat gray, cv::CascadeClassifier cas) {
  std::vector<cv::Rect> faces;
  cv::Rect face;
  cas.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(80, 80));
  if (faces.empty()) return {};
  face = *max_element(
      faces.begin(), faces.end(),
      [](const cv::Rect& a, const cv::Rect& b) { return a.area() < b.area(); });
  return face;
}

// 通过 GStreamer 打开 SDP 推流
static cv::VideoCapture open_sdp_stream(const std::string& sdp_path) {
  std::string pipe = "filesrc location=" + sdp_path +
                     " ! sdpdemux "
                     "! rtpjitterbuffer latency=0 "
                     "! rtpvrawdepay ! videoconvert "
                     "! video/x-raw,format=BGR "
                     "! appsink sync=false drop=true max-buffers=1";
  cv::VideoCapture cap(pipe, cv::CAP_GSTREAMER);
  if (!cap.isOpened()) {
    log_message(ERROR_INIT_FAILED, "无法打开SDP推流 " + sdp_path);
    return cap;
  }
  return cap;
}

// 处理人脸图像
cv::Mat process_face_image(cv::Mat face) {
  cv::Mat gray, equalized, resized;

  // 转换为灰度图
  cv::cvtColor(face, gray, cv::COLOR_BGR2GRAY);

  // 直方图均衡化
  cv::equalizeHist(gray, equalized);

  // 调整大小
  cv::resize(equalized, resized, cv::Size(SAMPLE_SIZE, SAMPLE_SIZE));

  return resized;
}

// 初始化人脸识别系统
bool init_face_recognition() {
  // 查找级联文件
  std::string cascade_path = find_cascade();
  if (cascade_path.empty() || !face_cas.load(cascade_path)) {
    log_message(ERROR_INIT_FAILED,
                "找不到lbpcascade_frontalface_improved.xml文件路径");
    return false;
  }
  log_message(ERROR_SUCCESS, "级联分类器加载成功: " + cascade_path);

  // 初始化人脸识别器
  recog = cv::face::LBPHFaceRecognizer::create(1, 8, 4, 4, 8.0);
  log_message(ERROR_SUCCESS, "人脸识别器初始化成功");

  // 打开视频流
  std::string sdp = "/tmp/yuyv.sdp";
  cap = open_sdp_stream(sdp);
  if (!cap.isOpened()) {
    log_message(ERROR_INIT_FAILED, "打不开SDP推流 " + sdp + "，回退到摄像头0");
    cap.open(0);
  }

  if (!cap.isOpened()) {
    log_message(ERROR_INIT_FAILED, "无法打开任何视频源");
    return false;
  }
  log_message(ERROR_SUCCESS, "视频流打开成功");

  // 初始化数据库
  init_database();

  // 尝试加载已训练的模型
  if (std::filesystem::exists(MODEL_PATH)) {
    try {
      recog->read(MODEL_PATH);
      log_message(ERROR_SUCCESS, "成功加载已训练的模型: " + MODEL_PATH);
    } catch (const cv::Exception& e) {
      log_message(ERROR_MODEL_NOT_FOUND,
                  "加载模型失败: " + std::string(e.what()));
    }
  } else {
    log_message(ERROR_MODEL_NOT_FOUND, "未找到已训练的模型，将需要重新训练");
  }

  return true;
}

// 采集样本
bool capture_sample() {
  cv::Mat frame, gray, face, processed_face;

  if (!cap.read(frame) || frame.empty()) {
    log_message(ERROR_CAPTURE_FAILED, "无法获取视频帧");
    return false;
  }

  // 转换为灰度图
  cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

  // 检测人脸
  cv::Rect face_rect = detect_face(gray, face_cas);
  if (face_rect.empty()) {
    log_message(ERROR_CAPTURE_FAILED, "未检测到人脸");
    return false;
  }

  // 提取人脸
  face = frame(face_rect);

  // 处理人脸图像
  processed_face = process_face_image(face);

  // 保存样本
  samples.push_back(processed_face);
  labels.push_back(current_user_id);
  sample_count++;

  log_message(ERROR_SUCCESS,
              "样本采集成功，当前样本数: " + std::to_string(sample_count));

  return true;
}

// 训练模型
bool train_model() {
  if (samples.empty()) {
    log_message(ERROR_TRAINING_FAILED, "没有足够的样本进行训练");
    return false;
  }

  log_message(ERROR_SUCCESS,
              "开始训练模型，样本数: " + std::to_string(samples.size()));

  try {
    recog->train(samples, labels);
    recog->save(MODEL_PATH);
    log_message(ERROR_SUCCESS, "模型训练成功，已保存到: " + MODEL_PATH);
    return true;
  } catch (const cv::Exception& e) {
    log_message(ERROR_TRAINING_FAILED,
                "模型训练失败: " + std::string(e.what()));
    return false;
  }
}

// 进行人脸识别
bool recognize_face(cv::Mat& frame, cv::Mat& gray) {
  cv::Rect face_rect = detect_face(gray, face_cas);
  if (face_rect.empty()) {
    return false;
  }

  // 提取和处理人脸
  cv::Mat face = frame(face_rect);
  cv::Mat processed_face = process_face_image(face);

  // 进行识别
  int predicted_label;
  double confidence;
  recog->predict(processed_face, predicted_label, confidence);

  // 绘制人脸框
  cv::rectangle(frame, face_rect, cv::Scalar(0, 255, 0), 2);

  // 处理识别结果
  if (confidence < 80) {
    // 识别成功
    std::string result_text;
    if (predicted_label >= 0 && predicted_label < (int)employee_names.size()) {
      result_text = employee_names[predicted_label] + " (" +
                    std::to_string((int)confidence) + ")";
    } else {
      result_text = "User " + std::to_string(predicted_label) + " (" +
                    std::to_string((int)confidence) + ")";
    }
    cv::putText(frame, result_text, cv::Point(face_rect.x, face_rect.y - 10),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
    log_message(ERROR_SUCCESS, "识别成功: " + result_text);
    return true;
  } else {
    // 识别失败
    std::string result_text =
        "Unknown (" + std::to_string((int)confidence) + ")";
    cv::putText(frame, result_text, cv::Point(face_rect.x, face_rect.y - 10),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
    log_message(ERROR_RECOGNITION_FAILED, "识别失败: " + result_text);
    return false;
  }
}

// 显示操作说明
void show_help() {
  std::cout << "操作说明：\n"
            << "  [1~5] 选择当前ID（默认 user1）\n"
            << "  [c]   采集当前帧的人脸样本\n"
            << "  [t]   训练（采集若干样本后）\n"
            << "  [r]   开/关识别显示\n"
            << "  [h]   显示帮助\n"
            << "  [Esc]   退出\n";
}

// 主函数
int main(int argc, char** argv) {
  // 初始化人脸识别系统
  if (!init_face_recognition()) {
    log_message(ERROR_INIT_FAILED, "人脸识别系统初始化失败");
    return -1;
  }

  // 显示操作说明
  show_help();

  // 主循环
  cv::Mat frame, gray;
  while (true) {
    // 读取视频帧
    if (!cap.read(frame) || frame.empty()) {
      if ((char)cv::waitKey(1) == 27) break;
      continue;
    }

    // 转换为灰度图
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    // 如果在识别模式，进行识别
    if (is_recognizing) {
      recognize_face(frame, gray);
    }

    // 显示当前状态
    std::string status_text =
        "当前用户: user" + std::to_string(current_user_id) +
        " | 识别模式: " + (is_recognizing ? "开启" : "关闭") +
        " | 样本数: " + std::to_string(samples.size());
    cv::putText(frame, status_text, cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX,
                0.5, cv::Scalar(255, 255, 255), 2);

    // 显示图像
    cv::imshow("Face Recognition", frame);

    // 处理键盘输入
    char key = (char)cv::waitKey(1);
    switch (key) {
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
        current_user_id = key - '0';
        current_user_name = "user" + std::to_string(current_user_id);
        log_message(ERROR_SUCCESS, "当前选择用户: " + current_user_name);
        break;
      case 'c':
        // 采集样本
        capture_sample();
        break;
      case 't':
        // 训练模型
        train_model();
        break;
      case 'r':
        // 切换识别模式
        is_recognizing = !is_recognizing;
        log_message(ERROR_SUCCESS, std::string("识别模式: ") +
                                       (is_recognizing ? std::string("开启")
                                                       : std::string("关闭")));
        break;
      case 'h':
        // 显示帮助
        show_help();
        break;
      case 27:  // ESC键退出
        log_message(ERROR_SUCCESS, "退出程序");
        cap.release();
        cv::destroyAllWindows();
        return 0;
    }
  }

  cap.release();
  cv::destroyAllWindows();
  return 0;
}
