cmake_minimum_required(VERSION 3.9)
project(a3sdn)

set(CMAKE_CXX_STANDARD 17)
add_executable(a3sdn a3sdn.cpp controller.cpp controller.h switch.cpp switch.h connection.cpp connection.h packet.cpp packet.h gate.cpp gate.h)
