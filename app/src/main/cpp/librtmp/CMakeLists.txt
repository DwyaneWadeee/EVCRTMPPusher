cmake_minimum_required(VERSION 3.18.1)

file(GLOB rtmp_src *.c)


#如果不修改源码的情况下，解决报错，添加宏
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_CRYPTO")

add_library( # Sets the name of the library.
        rtmp
        STATIC
        ${rtmp_src})

