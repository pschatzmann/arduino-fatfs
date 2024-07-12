#include "assert.h"
#include "fatfs.h"

RamIO drv{100, 512};  // 100 sector with 512 bytes
File file;

void setup() {
  Serial.begin(115200);
  while(!Serial);

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