/* Directory iterator test on a single RamIO drive.
 * Covers SDClass::directoryIterator() and SDClass::recursiveDirectoryIterator(),
 * which iterate the volume of the SD object they are called on.
 */
#include <string>
#include <vector>

#include "fatfs.h"
#include "filesystem.h"
#include "test_common.h"

using namespace fatfs;

RamIO drv{200, 512};

static bool contains(const std::vector<std::string>& v, const std::string& s) {
  for (const auto& e : v) {
    if (e == s) return true;
  }
  return false;
}

static void createFile(const char* path, const char* content) {
  File f = SD.open(path, FILE_WRITE);
  CHECK((bool)f, "could not create test file");
  f.write((const uint8_t*)content, strlen(content));
  f.flush();
  f.close();
}

void setup() {
  CHECK(SD.begin(drv), "SD.begin() failed");

  // Build the tree:
  //   /a.txt
  //   /dir/
  //   /dir/b.txt
  //   /dir/sub/
  //   /dir/sub/c.txt
  CHECK(SD.mkdir("/dir"), "mkdir /dir failed");
  CHECK(SD.mkdir("/dir/sub"), "mkdir /dir/sub failed");
  createFile("/a.txt", "a");
  createFile("/dir/b.txt", "b");
  createFile("/dir/sub/c.txt", "c");

  // non-recursive iterator on the root: a.txt and dir, but not nested files
  std::vector<std::string> root_entries;
  for (auto it = SD.directoryIterator("/");
       it != directory_iterator::end(); ++it) {
    root_entries.push_back((*it).path);
  }
  CHECK(root_entries.size() == 2, "directoryIterator(\"/\") returned wrong entry count");
  CHECK(contains(root_entries, "/a.txt"), "directoryIterator(\"/\") missing /a.txt");
  CHECK(contains(root_entries, "/dir"), "directoryIterator(\"/\") missing /dir");

  // non-recursive iterator on /dir: b.txt and sub, not c.txt
  std::vector<std::string> dir_entries;
  bool sub_is_dir = false;
  for (auto it = SD.directoryIterator("/dir");
       it != directory_iterator::end(); ++it) {
    auto e = *it;
    dir_entries.push_back(e.path);
    if (e.path == "/dir/sub") sub_is_dir = e.is_directory;
  }
  CHECK(dir_entries.size() == 2, "directoryIterator(\"/dir\") returned wrong entry count");
  CHECK(contains(dir_entries, "/dir/b.txt"), "directoryIterator(\"/dir\") missing /dir/b.txt");
  CHECK(contains(dir_entries, "/dir/sub"), "directoryIterator(\"/dir\") missing /dir/sub");
  CHECK(sub_is_dir, "/dir/sub not reported as a directory");

  // recursive iterator on the root: all 5 entries, including nested files
  std::vector<std::string> all_entries;
  for (auto it = SD.recursiveDirectoryIterator("/");
       it != recursive_directory_iterator::end(); ++it) {
    all_entries.push_back((*it).path);
  }
  CHECK(all_entries.size() == 5, "recursiveDirectoryIterator(\"/\") returned wrong entry count");
  CHECK(contains(all_entries, "/a.txt"), "recursive iterator missing /a.txt");
  CHECK(contains(all_entries, "/dir"), "recursive iterator missing /dir");
  CHECK(contains(all_entries, "/dir/b.txt"), "recursive iterator missing /dir/b.txt");
  CHECK(contains(all_entries, "/dir/sub"), "recursive iterator missing /dir/sub");
  CHECK(contains(all_entries, "/dir/sub/c.txt"), "recursive iterator missing /dir/sub/c.txt");

  // an empty iterator range on a non-existing directory
  int missing = 0;
  for (auto it = SD.directoryIterator("/does-not-exist");
       it != directory_iterator::end(); ++it) {
    missing++;
  }
  CHECK(missing == 0, "directoryIterator on missing path returned entries");

  printf("PASS: directory iterators on SDClass volume\n");
  TEST_EXIT_OK();
}

void loop() {}
