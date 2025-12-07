#include <stdio.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <opencv2/face.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using namespace cv;
using namespace filesystem;

// 级联分类器，优先项目目录中的(修改成LBP)
static string find_cascade() {
  vector<string> paths = {
      "./lbpcascade_frontalface_improved.xml",
      "/usr/share/opencv/lbpcascades/lbpcascade_frontalface_improved.xml",
      "/usr/share/opencv4/lbpcascades/lbpcascade_frontalface_improved.xml"};
  for (auto& p : paths)
    if (exists(p)) return p;
  return "";
}
// 返回图片中最大人脸(取消了灰度图处理，灰度图在前面图片预处理中完成)
static Rect detect_face(Mat gray, CascadeClassifier cas) {
  vector<Rect> faces;
  Rect face;
  cas.detectMultiScale(gray, faces, 1.1, 3, 0, Size(80, 80));
  if (faces.empty()) return {};
  face = *max_element(
      faces.begin(), faces.end(),
      [](const Rect& a, const Rect& b) { return a.area() < b.area(); });
  return face;
}
// 通过 GStreamer 打开 SDP 推流
static VideoCapture open_sdp_stream(const string& sdp_path) {
  string pipe = "filesrc location=" + sdp_path +
                " ! sdpdemux "
                "! rtpjitterbuffer latency=0 "
                "! rtpvrawdepay ! videoconvert "
                "! video/x-raw,format=BGR "
                "! appsink sync=false drop=true max-buffers=1";
  VideoCapture cap(pipe, CAP_GSTREAMER);
  return cap;
}
void main(int argc, char** argv) {
  String cascade_path = find_cascade();
  CascadeClassifier face_cas;
  if (cascade_path.empty() || !face_cas.load(cascade_path)) {
    cerr << "找不到lbpcascade_frontalface_improved.xml文件路径\n"
         << "可放在程序同目录" << endl;
  }
  string sdp = "/tmp/yuyv.sdp";
  VideoCapture cap = open_sdp_stream(sdp);
  if (!cap.isOpened()) {
    cerr << "[WARM]打不开SDP推流" << sdp << "回退到摄像头0" << endl;
    cap.open(0);
  }
  if (!cap.isOpened()) {
    cerr << "无法打开任何视频源" << endl;
    return;
  }
#if CV_VERSION_MAJOR >= 4
  try {
    std::cout << "[INFO] backend=" << cap.getBackendName() << "\n";
  } catch (...) {
  }
#endif
  cout << "[INFO]" << "width: " << cap.get(CAP_PROP_FRAME_WIDTH)
       << "height: " << cap.get(CAP_PROP_FRAME_HEIGHT)
       << "FPS: " << cap.get(CAP_PROP_FPS) << endl;

  Ptr<face::LBPHFaceRecognizer> recog =
      face::LBPHFaceRecognizer::create(1, 8, 8, 8, 80.0);
  vector<Mat> sample;
  // 链接数据层(待完成)
  vector<int> id;
  vector<string> name;
  // 信息检索部分

  // lvgl按键操作获取部分
  cout << "操作说明：\n"
       << "  [1~5] 选择当前ID（默认 user1）\n"
       << "  [c]   采集当前帧的人脸样本\n"
       << "  [t]   训练（采集若干样本后）\n"
       << "  [r]   开/关识别显示\n"
       << "  [Esc]   退出\n";
  bool show_recongnition = false;
  Mat frame;
  while (true) {
    if (!cap.read(frame) || frame.empty()) {
      if ((char)waitKey(1) == 27) break;
      continue;
    }
  }
}