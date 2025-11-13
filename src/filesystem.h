// Minimal std::filesystem-like shim on top of Arduino-FatFs
// This header provides a directory iterator to ease porting code that expects
// filesystem helpers. It is not a drop-in replacement for std::filesystem.
// Uses FatFs API directly (f_opendir, f_readdir, etc.) instead of SD wrapper.

#pragma once

#include <string>
#include "fatfs.h"

namespace fatfs {


/**
 * @brief Representation of a directory entry
 * @ingroup iterator
 */
struct directory_entry {
  std::string path;
  bool is_directory;
  uint64_t size;  // File size in bytes (0 for directories)
};

// Get the global FatFs instance from SD object
inline fatfs::FatFs* get_fatfs() { return SD.getFatFs(); }

/**
 * @brief Iterator for non-recursive directory traversal
 * 
 * Iterates through entries in a single directory level without descending
 * into subdirectories. Uses FatFs f_opendir/f_readdir API directly.
 * @ingroup iterator
 * @code
 * for (auto it = directory_iterator("/mydir"); 
 *      it != directory_iterator::end(); 
 *      ++it) {
 *   auto entry = *it;
 *   printf("%s: %llu bytes\n", entry.path.c_str(), entry.size);
 * }
 * @endcode
 */
class directory_iterator {
 public:
  directory_iterator() : end_flag(true) {}
  explicit directory_iterator(const std::string& rootPath)
      : root(rootPath), end_flag(false) {
    auto fs = get_fatfs();
    if (!fs) {
      end_flag = true;
      return;
    }
    this->fs = fs;

    // Open directory using FatFs API
    FRESULT res = fs->f_opendir(&dir, root.empty() ? "/" : root.c_str());
    if (res != FR_OK) {
      end_flag = true;
    } else {
      advance();
    }
  }

  ~directory_iterator() {
    if (!end_flag && fs) {
      fs->f_closedir(&dir);
    }
  }

  directory_iterator& operator++() {
    advance();
    return *this;
  }

  directory_entry operator*() const {
    directory_entry e;
    e.path = root;
    if (!root.empty() && root.back() != '/') e.path += "/";
    e.path += info.fname;
    e.is_directory = (info.fattrib & AM_DIR) != 0;
    e.size = info.fsize;
    return e;
  }

  bool operator!=(const directory_iterator& other) const {
    return end_flag != other.end_flag;
  }

  static directory_iterator end() { return directory_iterator(); }

 private:
  void advance() {
    if (!fs) {
      end_flag = true;
      return;
    }

    // Read next directory entry
    FRESULT res = fs->f_readdir(&dir, &info);
    if (res != FR_OK || info.fname[0] == 0) {
      // Error or no more entries
      fs->f_closedir(&dir);
      end_flag = true;
    }
  }

  fatfs::FatFs* fs = nullptr;
  DIR dir;
  FILINFO info;
  std::string root;
  bool end_flag{true};
};

/**
 * @brief Iterator for recursive directory tree traversal
 * 
 * Automatically traverses all subdirectories in depth-first order. Maintains
 * a stack of open directory handles to track the current position in the tree.
 * Uses FatFs f_opendir/f_readdir API directly.
 * @ingroup iterator
 * 
 * @code
 * // Walk entire directory tree with one simple loop
 * for (auto it = recursive_directory_iterator("/"); 
 *      it != recursive_directory_iterator::end(); 
 *      ++it) {
 *   auto entry = *it;
 *   if (entry.is_directory) {
 *     printf("[DIR]  %s\n", entry.path.c_str());
 *   } else {
 *     printf("[FILE] %s (%llu bytes)\n", entry.path.c_str(), entry.size);
 *   }
 * }
 * @endcode
 */
class recursive_directory_iterator {
 public:
  recursive_directory_iterator() : end_flag(true) {}
  explicit recursive_directory_iterator(const std::string& rootPath)
      : end_flag(false) {
    auto fs = get_fatfs();
    if (!fs) {
      end_flag = true;
      return;
    }
    this->fs = fs;

    // Start with root directory
    if (!push_directory(rootPath.empty() ? "/" : rootPath)) {
      end_flag = true;
    } else {
      advance();
    }
  }

  ~recursive_directory_iterator() {
    // Close all open directories
    while (!stack.empty()) {
      if (fs) fs->f_closedir(&stack.back().dir);
      stack.pop_back();
    }
  }

  recursive_directory_iterator& operator++() {
    advance();
    return *this;
  }

  directory_entry operator*() const { return current_entry; }

  bool operator!=(const recursive_directory_iterator& other) const {
    return end_flag != other.end_flag;
  }

  static recursive_directory_iterator end() {
    return recursive_directory_iterator();
  }

 private:
  struct DirLevel {
    DIR dir;
    std::string path;
  };

  bool push_directory(const std::string& path) {
    if (!fs) return false;

    DirLevel level;
    level.path = path;
    FRESULT res = fs->f_opendir(&level.dir, path.c_str());
    if (res == FR_OK) {
      stack.push_back(level);
      return true;
    }
    return false;
  }

  void advance() {
    if (!fs || stack.empty()) {
      end_flag = true;
      return;
    }

    FILINFO info;

    while (!stack.empty()) {
      auto& level = stack.back();

      // Read next entry in current directory
      FRESULT res = fs->f_readdir(&level.dir, &info);

      if (res != FR_OK || info.fname[0] == 0) {
        // No more entries in this directory, pop and continue with parent
        fs->f_closedir(&level.dir);
        stack.pop_back();
        continue;
      }

      // Skip . and .. entries
      if (strcmp(info.fname, ".") == 0 || strcmp(info.fname, "..") == 0) {
        continue;
      }

      // Build full path for current entry
      current_entry.path = level.path;
      if (!current_entry.path.empty() && current_entry.path.back() != '/') {
        current_entry.path += "/";
      }
      current_entry.path += info.fname;
      current_entry.is_directory = (info.fattrib & AM_DIR) != 0;
      current_entry.size = info.fsize;

      // If this is a directory, push it onto the stack for depth-first
      // traversal
      if (current_entry.is_directory) {
        push_directory(current_entry.path);
      }

      // Return current entry (directory or file)
      return;
    }

    // Stack is empty, we're done
    end_flag = true;
  }

  fatfs::FatFs* fs = nullptr;
  std::vector<DirLevel> stack;
  directory_entry current_entry;
  bool end_flag{true};
};

}  // namespace fatfs_fs
