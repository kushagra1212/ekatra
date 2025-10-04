#include "MergeManager.h"
#include "ProgressReporter/ProgressReporter.h"
#include <algorithm>
#include <fstream>
#include <iostream>

const std::map<std::string, fs::path> categoryMap = {
    // Media
    {".jpg", "Media/Images"},
    {".jpeg", "Media/Images"},
    {".png", "Media/Images"},
    {".gif", "Media/Images"},
    {".heic", "Media/Images"},
    {".webp", "Media/Images"},
    {".svg", "Media/Images"},
    {".mp4", "Media/Videos"},
    {".mov", "Media/Videos"},
    {".avi", "Media/Videos"},
    {".mkv", "Media/Videos"},
    {".webm", "Media/Videos"},
    // Documents
    {".pdf", "Documents/Text"},
    {".doc", "Documents/Text"},
    {".docx", "Documents/Text"},
    {".txt", "Documents/Text"},
    {".rtf", "Documents/Text"},
    {".pages", "Documents/Text"},
    {".xls", "Documents/Spreadsheets"},
    {".xlsx", "Documents/Spreadsheets"},
    {".csv", "Documents/Spreadsheets"},
    {".numbers", "Documents/Spreadsheets"},
    {".ppt", "Documents/Presentations"},
    {".pptx", "Documents/Presentations"},
    {".key", "Documents/Presentations"},
    // Other categories
    {".mp3", "Audio"},
    {".wav", "Audio"},
    {".aac", "Audio"},
    {".flac", "Audio"},
    {".m4a", "Audio"},
    {".zip", "Archives"},
    {".rar", "Archives"},
    {".7z", "Archives"},
    {".tar", "Archives"},
    {".gz", "Archives"},
    {".cpp", "Code"},
    {".h", "Code"},
    {".js", "Code"},
    {".py", "Code"},
    {".java", "Code"},
    {".html", "Code"},
    {".css", "Code"},
    {".exe", "Applications"},
    {".dmg", "Applications"},
    {".app", "Applications"}};

void MergeManager::scanOnly(const ProcessOptions &options) {
  loadCustomRules(options.rulesFile);

  if (!fs::exists(options.sourceA) || !fs::exists(options.sourceB)) {
    std::cerr << "Error: One or both source folders do not exist." << std::endl;
    return;
  }

  std::cout << "Scanning for all files..." << std::endl;
  std::vector<fs::path> allFiles;
  scanDirectory(options.sourceA, allFiles, options.includeHidden);
  scanDirectory(options.sourceB, allFiles, options.includeHidden);
  std::cout << "Found " << allFiles.size()
            << " files. Identifying uncategorized files..." << std::endl;

  std::vector<fs::path> uncategorizedFiles;
  for (const auto &filePath : allFiles) {
    fs::path targetDir = getDestinationForFile(filePath, options.destination);
    if (targetDir.empty()) {
      uncategorizedFiles.push_back(filePath);
    }
  }

  if (uncategorizedFiles.empty()) {
    std::cout << "Scan complete. All files are covered by existing rules."
              << std::endl;
    return;
  }

  std::cout << "Found " << uncategorizedFiles.size()
            << " uncategorized files. Writing paths to " << options.scanFile
            << "..." << std::endl;

  std::ofstream outFile(options.scanFile);
  if (!outFile.is_open()) {
    std::cerr << "Error: Could not open output file for writing: "
              << options.scanFile << std::endl;
    return;
  }

  for (const auto &path : uncategorizedFiles) {
    outFile << path.string() << "\n";
  }
  outFile.close();

  std::cout << "Scan complete. You can now use the generated file '"
            << options.scanFile << "' to create custom rules." << std::endl;
}

void MergeManager::loadCustomRules(const fs::path &rulesFilePath) {
  if (rulesFilePath.empty() || !fs::exists(rulesFilePath)) {
    return;
  }

  std::ifstream rulesFile(rulesFilePath);
  if (!rulesFile) {
    std::cerr << "Warning: Could not open rules file: "
              << rulesFilePath.string() << std::endl;
    return;
  }

  std::cout << "Loading custom sorting rules from: " << rulesFilePath.string()
            << std::endl;
  std::string line;
  int lineNum = 0;
  while (std::getline(rulesFile, line)) {
    lineNum++;
    // Ignore empty lines or comments
    if (line.empty() || line[0] == '#') {
      continue;
    }

    size_t delimiterPos = line.find(':');
    if (delimiterPos == std::string::npos || delimiterPos == 0) {
      std::cerr << "Warning: Invalid rule format on line " << lineNum
                << ". Skipping. Format should be 'regex:destination'"
                << std::endl;
      continue;
    }

    std::string regexStr = line.substr(0, delimiterPos);
    std::string destination = line.substr(delimiterPos + 1);

    try {
      // Compile the regex and store it with its destination
      m_customRules.emplace_back(std::regex(regexStr), destination);
    } catch (const std::regex_error &e) {
      std::cerr << "Warning: Invalid regex on line " << lineNum << ": '"
                << regexStr << "'. " << e.what() << ". Skipping." << std::endl;
    }
  }
}

void MergeManager::scanDirectory(const fs::path &sourceDir,
                                 std::vector<fs::path> &fileList,
                                 bool includeHidden) {
  for (const auto &entry : fs::recursive_directory_iterator(sourceDir)) {
    if (fs::is_regular_file(entry.symlink_status())) {

      if (!includeHidden && entry.path().filename().string()[0] == '.') {
        continue;
      }

      fileList.push_back(entry.path());
    }
  }
}

void MergeManager::copyFileWithProgress(
    const fs::path &from, const fs::path &to,
    const std::function<void(long long)> &onProgress) {
  const size_t bufferSize = 8192;
  char buffer[bufferSize];
  long long bytesCopied = 0;

  std::ifstream in(from, std::ios::binary);
  std::ofstream out(to, std::ios::binary);

  while (in.read(buffer, bufferSize)) {
    out.write(buffer, in.gcount());
    bytesCopied += in.gcount();
    onProgress(bytesCopied);
  }
  out.write(buffer, in.gcount());
  bytesCopied += in.gcount();
  onProgress(bytesCopied); // Final update
}

void MergeManager::process(const ProcessOptions &options) {

  loadCustomRules(options.rulesFile);

  if (!fs::exists(options.sourceA) || !fs::exists(options.sourceB)) {
    std::cerr << "Error: One or both source folders do not exist." << std::endl;
    return;
  }

  ProgressReporter reporter;

  reporter.reportScanBegin();
  std::vector<fs::path> allFiles;
  scanDirectory(options.sourceA, allFiles, options.includeHidden);
  scanDirectory(options.sourceB, allFiles, options.includeHidden);
  long long totalSize = 0;
  for (const auto &file : allFiles) {
    totalSize += fs::file_size(file);
  }
  reporter.reportScanComplete(allFiles.size(), totalSize);

  reporter.startProcessing();

  try {
    fs::create_directories(options.destination);
    for (const auto &filePath : allFiles) {
      fs::path targetDir = getDestinationForFile(filePath, options.destination);
      if (targetDir.empty()) {
        targetDir = reporter.promptForUnknownFile(filePath, options.destination,
                                                  m_userRules, m_customRules);
      }
      fs::create_directories(targetDir);

      fs::path destFile;
      if (options.skipDuplicates) {
        destFile = targetDir / filePath.filename();
        if (fs::exists(destFile)) {
          reporter.reportFileProcessed(filePath);
          continue;
        }
      } else {
        destFile = getUniquePath(targetDir / filePath.filename());
      }

      std::error_code ec;
      if (options.operation == Operation::Copy) {
        reporter.startFile(filePath);
        copyFileWithProgress(filePath, destFile, [&](long long bytes) {
          reporter.updateFileProgress(bytes);
        });
        reporter.finishFile();
      } else { // Operation::Move
        reporter.reportFileProcessed(filePath);
        fs::rename(filePath, destFile, ec);
      }

      if (ec) {
        std::cerr << "\nError processing " << filePath.string() << ": "
                  << ec.message() << std::endl;
      }
    }
    reporter.finishProcessing();
  } catch (const fs::filesystem_error &e) {
    std::cerr << "\nFatal error: " << e.what() << std::endl;
  }
}

fs::path MergeManager::getDestinationForFile(const fs::path &file,
                                             const fs::path &destBaseDir) {

  std::string filename = file.filename().string();

  for (const auto &rule : m_customRules) {
    if (std::regex_match(filename, rule.first)) {
      return destBaseDir / rule.second;
    }
  }

  std::string ext = file.extension().string();
  if (ext.empty())
    return fs::path();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  auto it = categoryMap.find(ext);
  if (it != categoryMap.end()) {
    return destBaseDir / it->second;
  }
  auto userIt = m_userRules.find(ext);
  if (userIt != m_userRules.end()) {
    return destBaseDir / userIt->second;
  }
  return fs::path();
}

fs::path MergeManager::getUniquePath(const fs::path &targetPath) {
  if (!fs::exists(targetPath))
    return targetPath;
  fs::path newPath = targetPath;
  const std::string stem = targetPath.stem().string();
  const std::string extension = targetPath.extension().string();
  int counter = 1;
  while (fs::exists(newPath)) {
    newPath = targetPath.parent_path() /
              (stem + "_" + std::to_string(counter++) + extension);
  }
  return newPath;
}
