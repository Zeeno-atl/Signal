cmake_minimum_required(VERSION 3.5)
project("Signal")

add_library(${PROJECT_NAME} INTERFACE)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "submodule/3p")

# TODO we are missing folder inside: include/signal/*.hpp
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_SOURCE_DIR}/submodules/Signal/include)
