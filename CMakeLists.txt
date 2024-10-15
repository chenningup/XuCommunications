cmake_minimum_required(VERSION 3.10)
project(XuCommunications)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core)
# 指定库的源文件和头文件
file(GLOB_RECURSE SOURCES "Src/**/*.cpp")
file(GLOB_RECURSE HEADERS "Src/**/*.h")

add_library(XuCommunications STATIC  ${SOURCES} ${HEADERS}
    Src/xudefine.h)

target_link_libraries(XuCommunications Qt6::Core)
# 创建共享库

if(WIN32)
    set_target_properties(XuCommunications PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../Libs/Windows/$<CONFIG>/"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../Libs/Windows/$<CONFIG>/"
    )
elseif(UNIX)
    set_target_properties(XuCommunications PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../Libs/Linux/$<CONFIG>/"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../Libs/Linux/$<CONFIG>/"
    )
endif()
# 设置库的包含目录
target_include_directories(XuCommunications PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/3rd" "${CMAKE_CURRENT_SOURCE_DIR}/Src")
