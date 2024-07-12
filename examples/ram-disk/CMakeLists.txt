cmake_minimum_required(VERSION 3.16)

# set the project name
project(ram_disk LANGUAGES CXX)

set (DCMAKE_CXX_FLAGS -Werror -Wall -Wextra)
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

add_compile_options("-xc++")
SET_SOURCE_FILES_PROPERTIES(ram-disk.ino PROPERTIES LANGUAGE CXX)

# build sketch as executable  
add_executable (ram_disk ram-disk.ino )

# link with fatfs
target_link_libraries(ram_disk PUBLIC arduino_fatfs)