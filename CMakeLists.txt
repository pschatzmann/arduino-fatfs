cmake_minimum_required(VERSION 3.16)

# set the project name
project(arduino_fatfs)

# lots of warnings and all warnings as errors
## add_compile_options(-Wall -Wextra )
set(CMAKE_CXX_STANDARD 17)
set (DCMAKE_CXX_FLAGS -Werror -Wall -Wextra)

option(FATFS_EXAMPES "build examples" ON)

file(GLOB_RECURSE SRC_LIST_CPP CONFIGURE_DEPENDS  "${PROJECT_SOURCE_DIR}/src/*.cpp" )

# define libraries
add_library (arduino_fatfs STATIC ${SRC_LIST_CPP})

# define location for header files
target_include_directories(arduino_fatfs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src  )
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

# build examples
if(FATFS_EXAMPES)
  add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/examples/ram-disk")
#  add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/examples/driver-test")
endif()