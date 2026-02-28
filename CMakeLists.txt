# 1. 指定 CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 2. 定义项目名称和 C++ 标准
project(MiniRpCCpp VERSION 1.0 LANGUAGES CXX)

# 设置 C++ 标准为 C++11 (你的代码需要 C++11 的 lambda 和 thread)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 3. 收集源文件
# 自动查找 src 目录下所有的 .cpp 文件
file(GLOB SOURCES "src/*.cpp")

# 4. 定义可执行文件
# 生成的可执行文件名叫 rpc_server，由 SOURCES 列表中的文件编译而成
add_executable(rpc_server ${SOURCES})

# 5. 包含头文件目录
# 告诉编译器去 ./include 目录找头文件 (这样 #include "rpc_provider.h" 就能找到了)
target_include_directories(rpc_server PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 6. 链接线程库 (pthread)
# C++11 的 std::thread 在 Linux 下需要链接 pthread
find_package(Threads REQUIRED)
target_link_libraries(rpc_server PRIVATE Threads::Threads)

# 7. (可选) 打印一些信息，方便调试
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Source files: ${SOURCES}")

# 8. (可选) 设置编译优化选项
# 如果是 Debug 模式，加 -g；如果是 Release 模式，加 -O3
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building in Debug mode with symbols")
    target_compile_options(rpc_server PRIVATE -g -Wall)
else()
    message(STATUS "Building in Release mode with optimizations")
    target_compile_options(rpc_server PRIVATE -O3 -Wall)
endif()
