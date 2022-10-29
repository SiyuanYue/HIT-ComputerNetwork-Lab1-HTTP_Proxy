cmake_minimum_required(VERSION 3.21)
project(Lab1)
project(ll)
project(test)
set(CMAKE_CXX_STANDARD 20)

add_executable(Lab1 lab1.cpp HttpProxy.cpp HttpProxy.h ProxyServerSocket.cpp ProxyServerSocket.h HttpHeader.cpp HttpHeader.h)
TARGET_LINK_LIBRARIES(Lab1 wsock32)
