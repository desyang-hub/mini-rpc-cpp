# 1. 指定 CMake 最低版本要求
cmake_minimum_required(VERSION 3.16)

# 2. 定义项目名称和 C++ 标准
project(MiniRpCCpp VERSION 1.0 LANGUAGES CXX)

# 设置 C++ 标准为 C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")

# ============ 手动配置 Protobuf ============
if(DEFINED ENV{CONDA_PREFIX})
    set(CONDA_ROOT $ENV{CONDA_PREFIX})
    message(STATUS "Detected Conda Environment: ${CONDA_ROOT}")
    
    # 直接设置路径变量
    set(PROTOBUF_INCLUDE_DIR "${CONDA_ROOT}/include")
    set(PROTOBUF_LIBRARY "${CONDA_ROOT}/lib/libprotobuf.so")
    set(PROTOBUF_PROTOC_EXECUTABLE "${CONDA_ROOT}/bin/protoc")
    
    # 验证路径是否存在
    if(NOT EXISTS "${PROTOBUF_INCLUDE_DIR}/google/protobuf/message.h")
        message(FATAL_ERROR "Protobuf headers not found at ${PROTOBUF_INCLUDE_DIR}")
    endif()
    
    if(NOT EXISTS "${PROTOBUF_LIBRARY}")
        message(FATAL_ERROR "Protobuf library not found at ${PROTOBUF_LIBRARY}")
    endif()
    
    # 手动创建导入目标
    add_library(protobuf::libprotobuf UNKNOWN IMPORTED)
    set_target_properties(protobuf::libprotobuf PROPERTIES
        IMPORTED_LOCATION "${PROTOBUF_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PROTOBUF_INCLUDE_DIR}"
    )
    
    # 手动创建 protoc 可执行文件目标
    add_executable(protobuf::protoc IMPORTED)
    set_target_properties(protobuf::protoc PROPERTIES
        IMPORTED_LOCATION "${PROTOBUF_PROTOC_EXECUTABLE}"
    )
    
    # 设置变量，让后续代码知道 Protobuf 已就绪
    set(Protobuf_FOUND TRUE)
    set(Protobuf_INCLUDE_DIRS "${PROTOBUF_INCLUDE_DIR}")
    set(Protobuf_LIBRARIES "${PROTOBUF_LIBRARY}")
    set(Protobuf_PROTOC_EXECUTABLE "${PROTOBUF_PROTOC_EXECUTABLE}")
    
    message(STATUS "✅ Manually configured Protobuf")
    message(STATUS "  Include: ${Protobuf_INCLUDE_DIRS}")
    message(STATUS "  Library: ${Protobuf_LIBRARIES}")
    message(STATUS "  Protoc: ${Protobuf_PROTOC_EXECUTABLE}")
else()
    message(FATAL_ERROR "No CONDA_PREFIX environment variable found. Please activate conda environment.")
endif()
# ============ 手动配置结束 ============

# 手动实现 protobuf_generate 功能
set(PROTO_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# 创建主输出目录
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

# 查找所有 .proto 文件
file(GLOB_RECURSE PROTO_FILES "${PROTO_ROOT_DIR}/*.proto")
message(STATUS "Found proto files: ${PROTO_FILES}")

# 为每个 .proto 文件生成输出文件列表
set(GENERATED_SOURCES "")
set(GENERATED_HEADERS "")

foreach(PROTO_FILE ${PROTO_FILES})
    # 获取相对路径
    file(RELATIVE_PATH PROTO_REL_PATH ${PROTO_ROOT_DIR} ${PROTO_FILE})
    
    # 获取文件名（不含扩展名）
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    
    # 获取目录结构
    get_filename_component(PROTO_DIR ${PROTO_FILE} DIRECTORY)
    file(RELATIVE_PATH PROTO_REL_DIR ${PROTO_ROOT_DIR} ${PROTO_DIR})
    
    # 设置生成的文件名
    set(GEN_SRC "${PROTO_GEN_DIR}/${PROTO_REL_DIR}/${PROTO_NAME}.pb.cc")
    set(GEN_HDR "${PROTO_GEN_DIR}/${PROTO_REL_DIR}/${PROTO_NAME}.pb.h")
    
    list(APPEND GENERATED_SOURCES ${GEN_SRC})
    list(APPEND GENERATED_HEADERS ${GEN_HDR})
    
    # 获取输出目录
    get_filename_component(GEN_OUTPUT_DIR ${GEN_SRC} DIRECTORY)
    
    # 添加自定义命令来生成代码
    add_custom_command(
        OUTPUT ${GEN_SRC} ${GEN_HDR}
        # 先创建目录，再运行 protoc
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GEN_OUTPUT_DIR}
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
            --proto_path=${PROTO_ROOT_DIR}
            --cpp_out=${PROTO_GEN_DIR}
            ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating protobuf code for ${PROTO_FILE}"
        VERBATIM
    )
endforeach()

# 创建一个对象库来打包生成的代码
add_library(proto_objects OBJECT ${GENERATED_SOURCES})

# 确保生成的头文件在构建时可用
target_include_directories(proto_objects
    PUBLIC
        ${PROTO_GEN_DIR}
        ${Protobuf_INCLUDE_DIRS}
)

# 链接 Protobuf 库到对象库
target_link_libraries(proto_objects
    PUBLIC
        protobuf::libprotobuf
)

# 收集源文件
file(GLOB SOURCES "src/*.cpp")
message(STATUS "Source files: ${SOURCES}")

# 定义可执行文件
add_executable(rpc_server ${SOURCES})

# 包含头文件目录
target_include_directories(rpc_server 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROTO_GEN_DIR}
)

# 查找并链接线程库
find_package(Threads REQUIRED)

# 链接必要的库
target_link_libraries(rpc_server
    PRIVATE
        proto_objects
        Threads::Threads
)

# 设置编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building in Debug mode with symbols")
    target_compile_options(rpc_server PRIVATE -g -Wall)
else()
    message(STATUS "Building in Release mode with optimizations")
    target_compile_options(rpc_server PRIVATE -O3 -Wall)
endif()

# 添加对生成文件的依赖
add_dependencies(rpc_server proto_objects)