/*----------------------------------------------------------------------/
/ Storage Speed Test for FatFS                                          /
/-----------------------------------------------------------------------/
/ Tests sequential and random read/write performance
*/

#include <SPI.h>
#include <stdio.h>
#include <string.h>
#include "fatfs.h"

// Uncomment ONE of the following to select the driver to test:

// Option 1: RAM disk
// RamIO drv{2000, 512};  // 2000 sectors = ~1MB

// Option 2: SD card with hardware SPI (default)
// If you want to test the RAM disk instead, uncomment the RamIO line above
// and comment out the SD section below.
#define MISO 2
#define MOSI 15
#define SCLK 14
#define CS 13

// Test configuration
#define TEST_FILE_SIZE_KB   100      // Size of test file in KB
#define BUFFER_SIZE         512      // Buffer size for operations
#define NUM_ITERATIONS      10       // Number of test iterations

static uint8_t buffer[BUFFER_SIZE];

void fillBuffer(uint8_t* buf, size_t size, uint8_t pattern) {
  for (size_t i = 0; i < size; i++) {
    buf[i] = (uint8_t)(pattern + i);
  }
}

bool verifyBuffer(uint8_t* buf, size_t size, uint8_t pattern) {
  for (size_t i = 0; i < size; i++) {
    if (buf[i] != (uint8_t)(pattern + i)) {
      return false;
    }
  }
  return true;
}

void printSpeed(const char* operation, unsigned long bytes, unsigned long timeMs) {
  float seconds = timeMs / 1000.0;
  float kb = bytes / 1024.0;
  float speed = kb / seconds;
  
  printf("%s: %lu bytes in %lu ms (%.2f KB/s)\n", 
         operation, bytes, timeMs, speed);
}

void testSequentialWrite() {
  printf("\n==== Sequential Write Test ====\n");
  
  File file = SD.open("speedtest.dat", FILE_WRITE);
  if (!file) {
    printf("Failed to open file for writing!\n");
    return;
  }

  unsigned long totalBytes = TEST_FILE_SIZE_KB * 1024;
  unsigned long bytesWritten = 0;
  uint8_t pattern = 0;
  
  unsigned long startTime = millis();
  
  while (bytesWritten < totalBytes) {
    fillBuffer(buffer, BUFFER_SIZE, pattern++);
    size_t written = file.write(buffer, BUFFER_SIZE);
    if (written != BUFFER_SIZE) {
      printf("Write error at byte %lu\n", bytesWritten);
      break;
    }
    bytesWritten += written;
  }
  
  file.flush();
  unsigned long endTime = millis();
  file.close();
  
  printSpeed("Sequential Write", bytesWritten, endTime - startTime);
}

void testSequentialRead() {
  printf("\n==== Sequential Read Test ====\n");
  
  File file = SD.open("speedtest.dat", FILE_READ);
  if (!file) {
    printf("Failed to open file for reading!\n");
    return;
  }

  unsigned long totalBytes = file.size();
  unsigned long bytesRead = 0;
  uint8_t pattern = 0;
  bool verified = true;
  
  unsigned long startTime = millis();
  
  while (bytesRead < totalBytes) {
    size_t toRead = min((unsigned long)BUFFER_SIZE, totalBytes - bytesRead);
    size_t read = file.read(buffer, toRead);
    if (read != toRead) {
      printf("Read error at byte %lu\n", bytesRead);
      break;
    }
    
    // Verify data
    if (!verifyBuffer(buffer, toRead, pattern++)) {
      printf("Data verification failed at byte %lu\n", bytesRead);
      verified = false;
      break;
    }
    
    bytesRead += read;
  }
  
  unsigned long endTime = millis();
  file.close();
  
  printSpeed("Sequential Read", bytesRead, endTime - startTime);
  if (verified) {
    printf("Data verification: PASSED\n");
  } else {
    printf("Data verification: FAILED\n");
  }
}

void testRandomWrite() {
  printf("\n==== Random Write Test ====\n");
  
  File file = SD.open("speedtest.dat", FILE_WRITE);
  if (!file) {
    printf("Failed to open file for writing!\n");
    return;
  }

  unsigned long fileSize = file.size();
  unsigned long numBlocks = fileSize / BUFFER_SIZE;
  unsigned long totalBytes = 0;
  
  unsigned long startTime = millis();
  
  for (int i = 0; i < NUM_ITERATIONS; i++) {
    // Random position
    unsigned long blockNum = random(numBlocks);
    unsigned long pos = blockNum * BUFFER_SIZE;
    
    file.seek(pos);
    fillBuffer(buffer, BUFFER_SIZE, (uint8_t)i);
    size_t written = file.write(buffer, BUFFER_SIZE);
    
    if (written != BUFFER_SIZE) {
      printf("Random write error at position %lu\n", pos);
      break;
    }
    totalBytes += written;
  }
  
  file.flush();
  unsigned long endTime = millis();
  file.close();
  
  printSpeed("Random Write", totalBytes, endTime - startTime);
  printf("Average seek + write time: %.2f ms\n", 
         (endTime - startTime) / (float)NUM_ITERATIONS);
}

void testRandomRead() {
  printf("\n==== Random Read Test ====\n");
  
  File file = SD.open("speedtest.dat", FILE_READ);
  if (!file) {
    printf("Failed to open file for reading!\n");
    return;
  }

  unsigned long fileSize = file.size();
  unsigned long numBlocks = fileSize / BUFFER_SIZE;
  unsigned long totalBytes = 0;
  
  unsigned long startTime = millis();
  
  for (int i = 0; i < NUM_ITERATIONS; i++) {
    // Random position
    unsigned long blockNum = random(numBlocks);
    unsigned long pos = blockNum * BUFFER_SIZE;
    
    file.seek(pos);
    size_t read = file.read(buffer, BUFFER_SIZE);
    
    if (read != BUFFER_SIZE) {
      printf("Random read error at position %lu\n", pos);
      break;
    }
    totalBytes += read;
  }
  
  unsigned long endTime = millis();
  file.close();
  
  printSpeed("Random Read", totalBytes, endTime - startTime);
  printf("Average seek + read time: %.2f ms\n", 
         (endTime - startTime) / (float)NUM_ITERATIONS);
}

void testFileOperations() {
  printf("\n==== File Operations Test ====\n");
  
  // Test file creation
  unsigned long startTime = millis();
  File file = SD.open("test_create.txt", FILE_WRITE);
  unsigned long createTime = millis() - startTime;
  if (file) {
    file.write((uint8_t)'x');  // Write at least one byte so file isn't empty
    file.close();
    printf("File create: %lu ms\n", createTime);
  } else {
    printf("File create: FAILED\n");
  }
  
  // Test file open
  startTime = millis();
  file = SD.open("test_create.txt", FILE_READ);
  unsigned long openTime = millis() - startTime;
  if (file) {
    file.close();
    printf("File open: %lu ms\n", openTime);
  } else {
    printf("File open: FAILED\n");
  }
  
  // Test file delete
  startTime = millis();
  bool deleted = SD.remove("test_create.txt");
  unsigned long deleteTime = millis() - startTime;
  if (deleted) {
    printf("File delete: %lu ms\n", deleteTime);
  } else {
    printf("File delete: FAILED\n");
  }
}

void printSystemInfo() {
  printf("\n==== System Information ====\n");
  printf("Buffer size: %d bytes\n", BUFFER_SIZE);
  printf("Test file size: %d KB\n", TEST_FILE_SIZE_KB);
  printf("Random I/O iterations: %d\n", NUM_ITERATIONS);
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  printf("\n");
  printf("========================================\n");
  printf("       FatFS Storage Speed Test\n");
  printf("========================================\n");

  // Initialize hardware SPI for SD card (default path)
  printf("Initializing SD card...\n");
  SPI.begin(SCLK, MISO, MOSI);
  delay(100);
  if (!SD.begin(CS, SPI)) {  // uses internal SDArduinoSpiIO driver
    printf("SD card initialization failed!\n");
    while (true);
  }
  printf("SD card initialized successfully!\n");

  printSystemInfo();
  
  // Clean up any existing test file
  SD.remove("speedtest.dat");

  // Quick filesystem presence check: try to create a tiny file; if it fails we
  // assume FR_NO_FILESYSTEM (13) was printed by the library and optionally format.
  {
    File probe = SD.open("fs_probe.tmp", FILE_WRITE);
    if (!probe) {
#if FF_USE_MKFS == 1 && FF_ARDUINO_LEVEL == 1
      printf("No filesystem detected. Attempting to format (mkfs)...\n");
      if (!SD.mkfs()) {
        printf("Formatting failed. Please format the card externally as FAT32 and retry.\n");
        while(true);
      }
      printf("Format complete. Re-mounting...\n");
      if (!SD.begin(CS, SPI)) {
        printf("Re-mount after format failed.\n");
        while(true);
      }
      // Re-test
      probe = SD.open("fs_probe.tmp", FILE_WRITE);
      if (!probe) {
        printf("Post-format probe still failing. Aborting tests.\n");
        while(true);
      }
      probe.close();
      SD.remove("fs_probe.tmp");
#else
      printf("No filesystem detected (likely FR_NO_FILESYSTEM). Please format the card as FAT/FAT32 on a PC and reinsert.\n");
      printf("Card must NOT be exFAT (FF_FS_EXFAT disabled).\n");
      while(true);
#endif
    } else {
      probe.close();
      SD.remove("fs_probe.tmp");
    }
  }
  
  // Run tests
  testSequentialWrite();
  testSequentialRead();
  testRandomWrite();
  testRandomRead();
  testFileOperations();
  
  // Clean up
  SD.remove("speedtest.dat");
  
  printf("\n========================================\n");
  printf("          All tests completed!\n");
  printf("========================================\n");
}

void loop() {
  // Nothing to do
}
