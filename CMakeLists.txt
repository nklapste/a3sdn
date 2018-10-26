cmake_minimum_required(VERSION 3.10)
project(a2sdn)

set(CMAKE_CXX_STANDARD 17)
add_executable(a2sdn a2sdn.cpp controller.cpp controller.h switch.cpp switch.h connection.cpp connection.h packet.cpp packet.h)
