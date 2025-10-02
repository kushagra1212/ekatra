#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class MergeManager {
public:
  enum class Operation { Copy, Move };

  void process(const fs::path &sourceA, const fs::path &sourceB,
               const fs::path &dest, Operation op, bool verbose,
               bool skipDuplicates);

               
  fs::path getDestinationForFile(const fs::path &file,
                                 const fs::path &destBaseDir);

private:
  void scanDirectory(const fs::path &sourceDir,
                     std::vector<fs::path> &fileList);

  void copyFileWithProgress(const fs::path &from, const fs::path &to,
                            const std::function<void(long long)> &onProgress);

  fs::path getUniquePath(const fs::path &targetPath);

  // This map stores user-defined rules for unknown file extensions.
  std::map<std::string, fs::path> m_userRules;
};

