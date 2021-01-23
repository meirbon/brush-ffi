cmake_minimum_required(VERSION 3.16)
project(opengl)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} src/main.cpp src/utils.cpp)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)

target_link_directories(${PROJECT_NAME} PRIVATE ../../target/release)
target_link_directories(${PROJECT_NAME} PRIVATE ../../target/debug)
target_include_directories(${PROJECT_NAME} PRIVATE ../../target)

if (UNIX)
	target_link_libraries(${PROJECT_NAME} PRIVATE libbrush.a)
	if (NOT APPLE)
		find_package(Threads REQUIRED)	
		target_link_libraries(${PROJECT_NAME} PRIVATE -ldl ${CMAKE_THREAD_LIBS_INIT})
	endif (NOT APPLE)
else (UNIX)
	target_link_libraries(${PROJECT_NAME} PRIVATE brush.lib)
endif (UNIX)

target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL glfw GLEW::GLEW)
target_include_directories(${PROJECT_NAME} PRIVATE OpenGL::GL glfw GLEW::GLEW glm)
