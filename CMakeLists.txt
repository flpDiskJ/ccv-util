cmake_minimum_required(VERSION 3.8)
project(ccv)

find_package(SDL2 REQUIRED)

set(SRC ${CMAKE_SOURCE_DIR}/src)


add_executable(ccv
	${SRC}/main.cpp
	${SRC}/ccv_image.cpp
	${SRC}/ccv2.cpp
)
target_include_directories(
	ccv PUBLIC
	${SDL2_INCLUDE_DIRS}
)
target_link_libraries(
	ccv PUBLIC
	${SDL2_LIBRARIES}
)
