cmake_minimum_required(VERSION 3.16)

# set the project name
project(driver_test LANGUAGES CXX)

set (DCMAKE_CXX_FLAGS -Werror -Wall -Wextra)
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

add_compile_options("-xc++")
SET_SOURCE_FILES_PROPERTIES(driver-test-sd.ino PROPERTIES LANGUAGE CXX)

# build sketch as executable  
add_executable (driver_test driver-test-sd.ino )

# link with fatfs
target_link_libraries(driver_test arduino_fatfs)