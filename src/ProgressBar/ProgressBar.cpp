#include "ProgressBar.h"
#include <iomanip>
#include <sstream> // For std::stringstream

void ProgressBar::start(long long total, std::string label) {
  m_total = total;
  m_label = std::move(label);
}

std::string ProgressBar::getString(long long current) {
  const int BAR_WIDTH = 50;
  float percentage =
      (m_total == 0) ? 1.0f : static_cast<float>(current) / m_total;
  int pos = static_cast<int>(BAR_WIDTH * percentage);

  std::stringstream ss;
  ss << m_label << " [";
  for (int i = 0; i < BAR_WIDTH; ++i) {
    if (i < pos)
      ss << "=";
    else
      ss << " ";
  }
  ss << "] " << std::fixed << std::setprecision(1) << percentage * 100.0
     << "% (" << formatBytes(current) << " / " << formatBytes(m_total) << ")";
  return ss.str();
}

std::string ProgressBar::formatBytes(long long bytes) {
  if (bytes < 1024)
    return std::to_string(bytes) + " B";
  if (bytes < 1024 * 1024)
    return std::to_string(bytes / 1024) + " KB";
  if (bytes < 1024 * 1024 * 1024)
    return std::to_string(bytes / (1024 * 1024)) + " MB";
  return std::to_string(bytes / (1024 * 1024 * 1024)) + " GB";
}
