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

void MergeManager::scanDirectory(const fs::path &sourceDir,
                                 std::vector<fs::path> &fileList) {
  for (const auto &entry : fs::recursive_directory_iterator(sourceDir)) {
    if (fs::is_regular_file(entry.symlink_status())) {
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

void MergeManager::process(const fs::path &sourceA, const fs::path &sourceB,
                           const fs::path &dest, Operation op, bool verbose,
                           bool skipDuplicates) {
  if (!fs::exists(sourceA) || !fs::exists(sourceB)) {
    std::cerr << "Error: One or both source folders do not exist." << std::endl;
    return;
  }

  ProgressReporter reporter;

  reporter.reportScanBegin();
  std::vector<fs::path> allFiles;
  scanDirectory(sourceA, allFiles);
  scanDirectory(sourceB, allFiles);
  long long totalSize = 0;
  for (const auto &file : allFiles) {
    totalSize += fs::file_size(file);
  }
  reporter.reportScanComplete(allFiles.size(), totalSize);

  reporter.startProcessing();

  try {
    fs::create_directories(dest);
    for (const auto &filePath : allFiles) {
      fs::path targetDir = getDestinationForFile(filePath, dest);
      if (targetDir.empty()) {
        targetDir = reporter.promptForUnknownFile(filePath, dest, m_userRules);
      }
      fs::create_directories(targetDir);

      fs::path destFile;
      if (skipDuplicates) {
        destFile = targetDir / filePath.filename();
        if (fs::exists(destFile)) {
          reporter.reportFileProcessed(filePath);
          continue;
        }
      } else {
        destFile = getUniquePath(targetDir / filePath.filename());
      }

      std::error_code ec;
      if (op == Operation::Copy) {
        reporter.startFile(filePath);
        copyFileWithProgress(filePath, destFile, [&](long long bytes) {
          reporter.updateFileProgress(bytes);
        });
        reporter.finishFile();
      } else { // Operation::Move
        reporter.reportFileProcessed(
            filePath); 
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

