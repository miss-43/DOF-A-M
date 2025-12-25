#include "app.h"
#include "face_recognition_main.cpp"

// 1. 初始化人脸识别系统
int face_recognize_init() {
    return init_face_recognition() ? ERROR_SUCCESS : ERROR_INIT_FAILED;
}

// 2. 选择用户
void face_recognize_select_user(int user_id) {
    send_event(EVENT_SELECT_USER, user_id);
}

// 3. 采集样本
int face_recognize_capture_sample() {
    return capture_sample() ? ERROR_SUCCESS : ERROR_CAPTURE_FAILED;
}

// 4. 训练模型
int face_recognize_train_model() {
    return train_model() ? ERROR_SUCCESS : ERROR_TRAINING_FAILED;
}

// 5. 切换识别模式
void face_recognize_toggle_recognition() {
    is_recognizing = !is_recognizing;
    log_message(ERROR_SUCCESS, "识别模式: " + (is_recognizing ? "开启" : "关闭"));
}

// 6. 获取识别状态
int face_recognize_is_recognizing() {
    return is_recognizing;
}

// 7. 获取当前用户
int face_recognize_get_current_user() {
    return current_user_id;
}

// 8. 获取样本数量
int face_recognize_get_sample_count() {
    return samples.size();
}

// 9. 获取运行状态
int face_recognize_is_running() {
    return running;
}

// 10. 退出系统
void face_recognize_exit() {
    send_event(EVENT_EXIT);
}

// 11. 显示帮助
void face_recognize_show_help() {
    show_help();
}