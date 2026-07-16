#pragma once
#include <cstdio>
#include <cstdlib>

// Explicit runtime check (unlike assert(), never compiled out by NDEBUG).
// Prints a diagnostic and terminates the process with exit code 1 on
// failure, so ctest reliably detects the failure via the process exit code.
//
// Uses _Exit() rather than exit(): these tests only use global RamIO/
// MultiIO/etc. objects to hold state for the duration of one process, never
// past it, so running their destructors on the way out buys nothing. It
// also sidesteps a flaky glibc dynamic-linker crash observed in some
// sandboxed environments during exit-time global destructor teardown.
#define CHECK(cond, msg)                                              \
  do {                                                                \
    if (!(cond)) {                                                    \
      printf("FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);          \
      fflush(stdout);                                                 \
      _Exit(1);                                                       \
    }                                                                 \
  } while (0)

#define TEST_EXIT_OK()   \
  do {                   \
    fflush(stdout);      \
    _Exit(0);            \
  } while (0)
