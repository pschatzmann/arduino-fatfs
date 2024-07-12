/**
 * Some simple but incomplete Arduino Emulation Classes to be used outside of Arduino
 */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <cstddef>
#include <string>

enum PrintCharFmt { DEC, HEX };

namespace fatfs {

using String = std::string;

class Print {
 public:
  virtual size_t write(uint8_t ch) {
    return write((const uint8_t *)&ch, 1);
  }

  virtual size_t write(const char *str) {
    return write((const uint8_t *)str, strlen(str));
  }

  virtual size_t write(const char *buffer, size_t size) {
    return write((const uint8_t *)buffer, size);
  }

  virtual int print(const char *msg) {
    int result = strlen(msg);
    return write(msg, result);
  }

  virtual int print(int number) {
    char msg[100];
    int len = sprintf(msg, "%d", number);
    return write(msg, len);
  }


  virtual int print(char c, PrintCharFmt spec) {
    char result[5];
    switch (spec) {
      case DEC:
        snprintf(result, 3, "%c", c);
        return print(result);
      case HEX:
        snprintf(result, 3, "%x", c);
        return print(result);
    }
    return -1;
  }

  virtual int println(const char *msg = "") {
    size_t len = strlen(msg)+1;
    char tmp[len];
    strcpy(tmp, msg);
    tmp[len-1]='\n';
    return write(tmp, len);
  }

  virtual int println(int n32) {
    char buffer[100] = {0};
    int result = snprintf(buffer, 100,"%d\n", n32);
    return write(buffer, result);
  }

  virtual size_t write(const uint8_t *data, size_t len) {
    if (data == nullptr) return 0;
    for (size_t j = 0; j < len; j++) {
      write(data[j]);
    }
    return len;
  }

  virtual int availableForWrite() { return 1024; }

  virtual void flush() { /* Empty implementation for backward compatibility */ }

 protected:
  int _timeout = 10;
};

class Stream : public Print {
 public:
  virtual int available() { return 0; }
  virtual size_t readBytes(uint8_t *data, size_t len) { return 0; }
#ifndef DOXYGEN
  virtual int read() { return -1; }
  virtual int peek() { return -1; }
  virtual void setTimeout(size_t t) {}
#endif
  operator bool() { return true; }
};

class HardwareSerial : public Stream {
 public:
  bool begin(int speed) { return true; }

  size_t write(uint8_t ch) override { return putchar(ch); }
  virtual operator bool() { return true; }
};

static HardwareSerial Serial;


}  // namespace fatfs

#ifndef ARDUINO

void setup();
void loop();

int main() {
    setup();
    while(true) loop();
}

#endif
