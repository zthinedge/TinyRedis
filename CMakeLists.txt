cmake_minimum_required(VERSION 3.10)
project(TinyRedis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(tinyredis
    main.cpp
    src/core/sds.cpp
)

# 包含头文件目录
target_include_directories(tinyredis PRIVATE ${CMAKE_SOURCE_DIR}/include)
