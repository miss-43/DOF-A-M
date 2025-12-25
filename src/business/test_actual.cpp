#include <iostream>
#include <thread>
#include <chrono>

// 模拟事件系统
void send_event(int event_type, int param1 = 0) {
    std::cout << 
