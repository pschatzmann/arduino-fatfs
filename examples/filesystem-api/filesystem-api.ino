/*----------------------------------------------------------------------/
/ Filesystem API Example - Recursive Directory Tree Walk                /
/-----------------------------------------------------------------------/
/ Demonstrates using recursive_directory_iterator to walk a directory 
/ tree automatically. Uses FatFs API directly for file info.
*/

#include <SPI.h>
#include "fatfs.h"
#include "filesystem.h"

// Use the filesystem shim namespace
using namespace fatfs_fs;

// pins for SD card
#define MISO 2
#define MOSI 15
#define SCLK 14
#define CS 13

// Helper function to count directory depth for indentation
int getDepth(const std::string &path) {
  int depth = 0;
  for (char c : path) {
    if (c == '/') depth++;
  }
  return depth;
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  printf("\n");
  printf("========================================\n");
  printf("  FatFS Filesystem API Example\n");
  printf("========================================\n");

  // Initialize hardware SPI for SD card
  printf("Initializing SD card...\n");
  SPI.begin(SCLK, MISO, MOSI);
  delay(100);
  
  if (!SD.begin(CS, SPI)) {
    printf("SD card initialization failed!\n");
    while (true);
  }
  printf("SD card initialized successfully!\n\n");

  // Walk the entire root directory tree using recursive_directory_iterator
  printf("Directory tree starting from root:\n");
  printf("====================================\n");

  // Use recursive_directory_iterator - it automatically traverses subdirectories!
  for (auto it = recursive_directory_iterator("/"); 
       it != recursive_directory_iterator::end(); 
       ++it) {
    auto entry = *it;
    
    // Calculate indentation based on path depth
    int depth = getDepth(entry.path);
    std::string indent;
    for (int i = 0; i < depth; i++) {
      indent += "  ";
    }
    
    // Extract filename from path
    const char* name = entry.path.c_str();
    const char* lastSlash = strrchr(name, '/');
    const char* filename = lastSlash ? lastSlash + 1 : name;
    
    if (entry.is_directory) {
      printf("%s[DIR]  %s\n", indent.c_str(), filename);
    } else {
      printf("%s[FILE] %s (%llu bytes)\n", indent.c_str(), filename, entry.size);
    }
  }
  
  printf("\n====================================\n");
  printf("Tree walk completed!\n");
  printf("====================================\n");
}

void loop() {
  // Nothing to do
}

