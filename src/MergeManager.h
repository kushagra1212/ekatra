#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <regex>
#include <string>
#include <vector>


namespace fs = std::filesystem;

struct ProcessOptions {
  fs::path sourceA;
  fs::path sourceB;
  fs::path destination;
  enum class Operation { Copy, Move } operation = Operation::Copy;
  bool verbose = false;
  bool skipDuplicates = false;
  bool includeHidden = false;
  std::string rulesFile;
};

class MergeManager {
public:
  using Operation = ProcessOptions::Operation;

  void process(const ProcessOptions &options);

  fs::path getDestinationForFile(const fs::path &file,
                                 const fs::path &destBaseDir);

private:
  void scanDirectory(const fs::path &sourceDir, std::vector<fs::path> &fileList,
                     bool includeHidden);

  void copyFileWithProgress(const fs::path &from, const fs::path &to,
                            const std::function<void(long long)> &onProgress);

  void loadCustomRules(const fs::path &rulesFilePath);

  fs::path getUniquePath(const fs::path &targetPath);

  // This map stores user-defined rules for unknown file extensions.
  std::map<std::string, fs::path> m_userRules;

  // store the compiled regex rules
  std::vector<std::pair<std::regex, std::string>> m_customRules;
};
