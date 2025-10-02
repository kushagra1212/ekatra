#pragma once

#include <chrono>
#include <string>

class ProgressBar {
public:
  void start(long long total, std::string label);
  std::string getString(long long current);

  // Helper function to format bytes into a human-readable string.
  static std::string formatBytes(long long bytes);

private:
  long long m_total = 0;
  std::string m_label;
};
