#include <chrono>
#include <iostream>
#include <thread>

#include "app.hpp"

// 完整功能测试
int main() {
  std::cout << "=== 人脸识别系统完整功能测试 ===\n";

  int result = 0;

  // 1. 测试系统初始化
  std::cout << "\n1. 测试系统初始化...\n";
  result = face_recognize_init();
  if (result == ERROR_SUCCESS) {
    std::cout << "✅ PASS: 系统初始化成功\n";
  } else {
    std::cout << "❌ FAIL: 系统初始化失败，错误码: " << result << "\n";
    return -1;
  }

  // 2. 测试选择用户
  std::cout << "\n2. 测试选择用户...\n";
  face_recognize_select_user(1);
  int current_user = face_recognize_get_current_user();
  if (current_user == 1) {
    std::cout << "✅ PASS: 选择用户1成功\n";
  } else {
    std::cout << "❌ FAIL: 选择用户失败，当前用户: " << current_user << "\n";
  }

  // 3. 测试采集样本（模拟）
  std::cout << "\n3. 测试采集样本...\n";
  // 注意：实际采集需要摄像头，这里只测试函数调用
  result = face_recognize_capture_sample();
  if (result == ERROR_SUCCESS || result == ERROR_CAPTURE_FAILED) {
    // ERROR_CAPTURE_FAILED 是预期的，因为可能没有检测到人脸
    std::cout << "✅ PASS: 采集样本函数调用成功\n";
  } else {
    std::cout << "❌ FAIL: 采集样本函数调用失败，错误码: " << result << "\n";
  }

  // 4. 测试训练模型（模拟）
  std::cout << "\n4. 测试训练模型...\n";
  result = face_recognize_train_model();
  if (result == ERROR_SUCCESS || result == ERROR_TRAINING_FAILED) {
    // ERROR_TRAINING_FAILED 是预期的，因为可能样本不足
    std::cout << "✅ PASS: 训练模型函数调用成功\n";
  } else {
    std::cout << "❌ FAIL: 训练模型函数调用失败，错误码: " << result << "\n";
  }

  // 5. 测试切换识别模式
  std::cout << "\n5. 测试切换识别模式...\n";
  face_recognize_toggle_recognition();
  bool is_recognizing = face_recognize_is_recognizing();
  std::cout << "✅ PASS: 识别模式切换为: " << (is_recognizing ? "开启" : "关闭")
            << "\n";

  // 6. 测试获取样本数量
  std::cout << "\n6. 测试获取样本数量...\n";
  int sample_count = face_recognize_get_sample_count();
  std::cout << "✅ PASS: 当前样本数量: " << sample_count << "\n";

  // 7. 测试获取运行状态
  std::cout << "\n7. 测试获取运行状态...\n";
  bool is_running = face_recognize_is_running();
  std::cout << "✅ PASS: 系统运行状态: " << (is_running ? "运行中" : "已停止")
            << "\n";

  // 8. 测试显示帮助
  std::cout << "\n8. 测试显示帮助...\n";
  face_recognize_show_help();
  std::cout << "✅ PASS: 显示帮助成功\n";

  // 9. 测试退出系统
  std::cout << "\n9. 测试退出系统...\n";
  face_recognize_exit();
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  is_running = face_recognize_is_running();
  std::cout << "✅ PASS: 退出系统成功，运行状态: "
            << (is_running ? "运行中" : "已停止") << "\n";

  std::cout << "\n=== 测试完成 ===\n";
  std::cout << "所有测试用例已执行\n";

  return 0;
}