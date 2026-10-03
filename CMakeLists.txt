cmake_minimum_required(VERSION 3.10)
project(laboratorio6)

set(CMAKE_CXX_STANDARD 17)
add_compile_definitions(GL_SILENCE_DEPRECATION)

find_package(OpenGL REQUIRED)

if(APPLE)
    find_library(GLUT_LIB GLUT)
    set(GLUT_LIBRARIES ${GLUT_LIB})
else()
    find_package(GLUT REQUIRED)
endif()

add_executable(editor2d
    main.cpp
    render.cpp
    input.cpp
)
target_link_libraries(editor2d OpenGL::GL ${GLUT_LIBRARIES})
