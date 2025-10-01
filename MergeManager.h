#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class MergeManager {
public:
  enum class Operation { Copy, Move };

  void process(const fs::path &sourceA, const fs::path &sourceB,
               const fs::path &dest, Operation op, bool verbose);

private:
  void processDirectory(const fs::path &sourceDir, const fs::path &destBaseDir,
                        Operation op, bool verbose);
  fs::path getDestinationForFile(const fs::path &file,
                                 const fs::path &destBaseDir);
  fs::path getUniquePath(const fs::path &targetPath);
};