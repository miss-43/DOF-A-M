#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

// 错误码定义
enum ERROR_CODE {
    ERROR_SUCCESS = 0,
    ERROR_INIT_FAILED,
    ERROR_CAPTURE_FAILED,
    ERROR_TRAINING_FAILED,
    ERROR_RECOGNITION_FAILED,
    ERROR_MODEL_NOT_FOUND,
    ERROR_UNKNOWN
};

// 1. 初始化人脸识别系统
int face_recognize_init();

// 2. 选择用户
void face_recognize_select_user(int user_id);

// 3. 采集样本
int face_recognize_capture_sample();

// 4. 训练模型
int face_recognize_train_model();

// 5. 切换识别模式
void face_recognize_toggle_recognition();

// 6. 获取识别状态
int face_recognize_is_recognizing();

// 7. 获取当前用户
int face_recognize_get_current_user();

// 8. 获取样本数量
int face_recognize_get_sample_count();

// 9. 获取运行状态
int face_recognize_is_running();

// 10. 退出系统
void face_recognize_exit();

// 11. 显示帮助
void face_recognize_show_help();

#ifdef __cplusplus
}
#endif

#endif // APP_H