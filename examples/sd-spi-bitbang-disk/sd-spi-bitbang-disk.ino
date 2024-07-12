#include "fatfs.h"

#define MISO 12
#define MOSI 13
#define SCLK 14
#define CS 15

SDBitBangSPIIO drv(MISO, MOSI, SCLK, CS);
File file;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // start SD
  SD.begin(drv);

  // open and write file
  file = SD.open("test", FILE_WRITE);
  file.println("test");
  file.flush();

  // read back value
  file.seek(0);
  auto str = file.readStringUntil('\n');
  Serial.println(str);
  assert(str == "test");

  Serial.println(file.size());

  file.close();
}

void loop() {}