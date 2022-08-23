cmake_minimum_required(VERSION 3.18.1)

project("EVCPusher")
#导入子目录的CmakeList.txt rtmp静态库
add_subdirectory(librtmp)

#导入头文件的路径 自动查找头文件 <>
include_directories(librtmp libx264/include)
#include_directories(librtmp )

#设置X264库路径
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/libx264/libs/${CMAKE_ANDROID_ARCH_ABI}")

add_library(
        native-lib
        SHARED
        native-lib.cpp
        VideoChannel.cpp
)


target_link_libraries(
        native-lib
        rtmp
        x264
#        ${lrog-lib})
        log
#        x264
)