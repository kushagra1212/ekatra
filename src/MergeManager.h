#pragma once

#include <filesystem>
#include <map>
#include <string>

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
  void processDirectory(const fs::path &sourceDir, const fs::path &destBaseDir,
                        Operation op, bool verbose, bool skipDuplicates);
  fs::path getUniquePath(const fs::path &targetPath);

  fs::path handleUnknownFile(const fs::path &file, const fs::path &destBaseDir);

  std::map<std::string, fs::path> m_userRules;
};