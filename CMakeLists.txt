cmake_minimum_required(VERSION 3.11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Test)
add_executable(engage practicum.c)

find_package(PkgConfig REQUIRED)

find_package(OpenGL REQUIRED)

if(${EMSCRIPTEN})
	#emscripten always finds opengl and doesn't need link options
	set(USE_FLAGS "-s USE_SDL=2 -s USE_SDL_IMAGE=2")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS} -s FULL_ES2=1 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2")
	set(CMAKE_EXECUTABLE_SUFFIX .html)
else()
	find_package(SDL2 REQUIRED)
	#IMG_SaveJPG was added in 2.0.2, SavePNG was added 2.0.0
	pkg_check_modules(SDL2_image REQUIRED SDL2_image>=2.0.2)
	target_link_libraries(engage OpenGL::GL)

	#arbitrary old version, using basic functions there since ~0.4
	pkg_check_modules(gsl gsl>=1.1)

	if(gsl_FOUND)
		include_directories(${gsl_INCLUDE_DIRS})
		target_link_libraries(engage ${gsl_LIBRARIES})
		target_compile_definitions(engage PUBLIC HAVE_GSL)
	endif()
endif()

include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_image_INCLUDE_DIRS})

target_link_libraries(engage ${SDL2_LIBRARIES})
target_link_libraries(engage ${SDL2_image_LIBRARIES})

