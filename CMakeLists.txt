cmake_minimum_required(VERSION 3.9)
project(a3sdn)

set(CMAKE_CXX_STANDARD 17)
add_executable(a3sdn src/a3sdn.cpp src/controller.cpp src/controller.h src/switch.cpp src/switch.h src/connection.cpp src/connection.h src/packet.cpp
        src/packet.h src/gate.cpp src/gate.h src/flow.h src/flow.cpp src/trafficfile.cpp src/trafficfile.h src/address.cpp src/address.h src/port.h src/port.cpp
        src/switchid.cpp src/switchid.h src/iprange.cpp src/iprange.h)
