cmake_minimum_required(VERSION 3.11)

project(Test)

find_package(SDL2 REQUIRED)

find_package(OpenGL REQUIRED)

find_package(PkgConfig REQUIRED)

#IMG_SaveJPG was added in 2.0.2, SavePNG was added 2.0.0
pkg_check_modules(SDL2_image REQUIRED SDL2_image>=2.0.2)

include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_image_INCLUDE_DIRS})

add_executable(engage practicum.c)

target_compile_definitions(engage PUBLIC HAVE_OPENGL)

target_link_libraries(engage ${SDL2_LIBRARIES})
target_link_libraries(engage ${SDL2_image_LIBRARIES})
target_link_libraries(engage OpenGL::GL)

