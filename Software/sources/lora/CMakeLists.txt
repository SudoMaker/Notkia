cmake_minimum_required(VERSION 3.22)
project(sx126x_tool C)

set(CMAKE_C_STANDARD 99)

add_executable(sx126x_tool main.c sx126x.c sx126x_hal.c)
target_link_libraries(sx126x_tool pthread)

add_executable(lora main2.c sx126x.c sx126x_hal.c)
target_link_libraries(lora pthread)