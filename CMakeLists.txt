cmake_minimum_required(VERSION 3.16)
project(opencv_lbph_demo LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc videoio highgui objdetect)
# face 模块在 contrib 包里
find_package(OpenCV 4 REQUIRED COMPONENTS face)

add_executable(face_demo src/main.cpp)
target_link_libraries(face_demo PRIVATE ${OpenCV_LIBS})
