cmake_minimum_required(VERSION 3.6)
project(MyTFTP)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES mytftp.c)
add_executable(MyTFTP ${SOURCE_FILES})