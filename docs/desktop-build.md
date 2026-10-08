# Desktop Build

The library can also be built and executed on a desktop (Linux, macOS, Windows) with CMake. This is useful for running the examples and the test suite without any Arduino hardware.

## Requirements

- CMake 3.16 or newer
- A C++17 compiler (GCC or Clang)
- Make (or another CMake generator)

## Build

```bash
cd TinyFATFS
mkdir build
cd build
cmake ..
make
```

The top-level [`CMakeLists.txt`](../CMakeLists.txt) defines the following options (all `ON` by default):

| Option | Description |
|---|---|
| `FATFS_EXAMPES` | Build the desktop examples (`ram-disk`, `driver-test-ram`) |
| `FATFS_BUILD_TESTS` | Build the desktop ctest suite |
| `FATFS_SANITIZE` | Build with `-fsanitize=address` |

To disable an option, pass it on the `cmake` command line, e.g. `cmake -DFATFS_SANITIZE=OFF ..`.

## Run the Examples

After building, the example binaries are located in the `build` folder tree, e.g.:

```bash
./examples/ram-disk/ram-disk
./examples/driver-test-ram/driver-test-ram
```

## Run the Tests

The desktop test suite covers the hardware-free drivers (`RamIO`, `StreamIO`, `MultiIO`, `FileIO`), the `SDClass`/`File` layer on top of them, and `TinyUsbMscIO` (tested against a test-only stand-in for Adafruit TinyUSB). SPI and SDMMC drivers need real hardware and are not covered by the test suite.

Run all tests with CTest from the `build` folder:

```bash
ctest --output-on-failure
```

Or run an individual test binary directly:

```bash
./tests/test_ramio_diskio
```

## Documentation

- [Driver Overview](driver-overview.md)
- [Driver Code Examples](driver-examples.md)
