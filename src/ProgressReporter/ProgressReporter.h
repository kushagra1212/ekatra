#pragma once

#include "src/ProgressBar/ProgressBar.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>

namespace fs = std::filesystem;

// Manages all console output, including progress bars and user prompts.
class ProgressReporter {
public:
  void reportScanBegin();
  void reportScanComplete(size_t fileCount, long long totalSize);

  void startProcessing();
  void startFile(const fs::path &path);
  void updateFileProgress(long long bytes);
  void finishFile();
  void reportFileProcessed(const fs::path &path);
  void finishProcessing();

  fs::path promptForUnknownFile(const fs::path &file,
                                const fs::path &destBaseDir,
                                std::map<std::string, fs::path> &userRules);

private:
  void draw();

  ProgressBar m_overallBar;
  ProgressBar m_fileBar;
  long long m_totalSize = 0;
  long long m_processedSize = 0;
  long long m_fileBytesProcessed = 0;
  long long m_fileSize = 0;
  bool m_isCopyingFile = false;
  std::chrono::steady_clock::time_point m_lastDrawTime;
};
