cmake_minimum_required(VERSION 3.16)

# Desktop ctest suite: only covers hardware-free drivers (RamIO, StreamIO,
# MultiIO) and the SDClass/File layer on top of them. SPI/SDMMC drivers need
# real hardware and are exercised via the examples instead.

function(fatfs_add_test name)
  add_executable(${name} ${name}.cpp)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${name} PRIVATE arduino_fatfs)
  add_test(NAME ${name} COMMAND ${name})
  set_tests_properties(${name} PROPERTIES
    TIMEOUT 30
    # Works around a sandboxed-environment hang where ASan's link-order
    # self-check tries to re-exec the process; harmless when unset/unneeded.
    ENVIRONMENT "ASAN_OPTIONS=verify_asan_link_order=0"
  )
endfunction()

fatfs_add_test(test_ramio_diskio)
fatfs_add_test(test_sdclass_ramio)
fatfs_add_test(test_multiio)
fatfs_add_test(test_streamio)
fatfs_add_test(test_fileio)

# TinyUsbMscIO needs Adafruit_TinyUSB.h, which needs real USB hardware to be
# meaningful; exercised here against a minimal test-only stand-in instead
# (support/tinyusb_stub/Adafruit_TinyUSB.h) that mimics just the
# Adafruit_USBD_MSC surface TinyUsbMscIO calls.
fatfs_add_test(test_tinyusb_msc)
target_include_directories(test_tinyusb_msc PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/support/tinyusb_stub)
target_compile_definitions(test_tinyusb_msc PRIVATE USE_TINYUSB)
