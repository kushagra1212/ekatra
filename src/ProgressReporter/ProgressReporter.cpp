#include "ProgressReporter.h"
#include <iomanip>
#include <limits>
#include <sstream>

void ProgressReporter::reportScanBegin() {
  std::cout << "Scanning folders to calculate total size..." << std::endl;
}

void ProgressReporter::reportScanComplete(size_t fileCount,
                                          long long totalSize) {
  m_totalSize = totalSize;
  std::cout << "Scan complete. Found " << fileCount << " files ("
            << ProgressBar::formatBytes(totalSize) << ")." << std::endl;
}

void ProgressReporter::startProcessing() {
  m_overallBar.start(m_totalSize, "Total Progress"); 
  draw();
}

void ProgressReporter::startFile(const fs::path &path) {
  m_fileSize = fs::file_size(path);
  m_fileBytesProcessed = 0;
  m_isCopyingFile = true;

  std::string filename = path.filename().string();

  const int LABEL_WIDTH = 15;
  const std::string TRUNCATION_MARKER = "...";

  if (filename.length() > LABEL_WIDTH) {
    filename = filename.substr(0, LABEL_WIDTH - TRUNCATION_MARKER.length()) + TRUNCATION_MARKER;
  }

  std::stringstream ss;
  ss << std::left << std::setw(LABEL_WIDTH) << filename;
  std::string fileLabel = ss.str();


  m_fileBar.start(m_fileSize, fileLabel); 

  std::cout << std::endl;

  draw();
}

void ProgressReporter::updateFileProgress(long long bytes) {
  m_fileBytesProcessed = bytes;
  draw();
}

void ProgressReporter::finishFile() {
  m_processedSize += m_fileSize;
  m_isCopyingFile = false;
  // 1. Move the cursor up to the "Overall" progress line.
  std::cout << "\x1B[A";

  // 2. Clear the "Overall" line and redraw it with the latest progress.
  std::cout << "\r\x1B[K" << m_overallBar.getString(m_processedSize + m_fileBytesProcessed);
  
  // 3. Move the cursor down to the now-obsolete "File" progress line.
  std::cout << std::endl;

  // 4. Clear the "File" progress line completely.
  std::cout << "\r\x1B[K";

  // 5. Move the cursor back up to the end of the "Overall" progress line
  //    to be ready for the next update.
  std::cout << "\x1B[A";
  
  std::cout.flush(); // Ensure changes are written to the console.
}

void ProgressReporter::reportFileProcessed(const fs::path &path) {
  m_processedSize += fs::file_size(path);
  draw();
}

void ProgressReporter::finishProcessing() {
  draw(); // Final update to 100%
  std::cout << std::endl;
  std::cout << "\n Merge operation completed successfully!" << std::endl;
}

void ProgressReporter::draw() {
  // To avoid flickering, only redraw periodically.
  auto now = std::chrono::steady_clock::now();
  if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastDrawTime)
              .count() < 50 &&
      m_processedSize + m_fileBytesProcessed < m_totalSize) {
    return;
  }
  m_lastDrawTime = now;

  if (m_isCopyingFile) {
    // For a stable two-line display, move cursor up, clear line, draw, repeat.
    std::cout << "\x1B[A"; // ANSI escape code to move cursor up one line
    std::cout << "\r\x1B[K"; // Carriage return and clear line
    std::cout << m_overallBar.getString(m_processedSize + m_fileBytesProcessed);
    std::cout << std::endl;
    std::cout << "\r\x1B[K"; // Carriage return and clear line
    std::cout << m_fileBar.getString(m_fileBytesProcessed);
  } else {
    // For single-line display, just use carriage return.
    std::cout << "\r\x1B[K"; // Carriage return and clear line
    std::cout << m_overallBar.getString(m_processedSize);
  }
  std::cout.flush();
}

fs::path
ProgressReporter::promptForUnknownFile(const fs::path &file,
                                     const fs::path &destBaseDir,
                                     std::map<std::string, fs::path> &userRules) {
  std::cout << std::string(100, ' ') << '\r'; // Clear progress line
  std::string ext = file.extension().string();
  std::cout << "\n--------------------------------------------------"
            << std::endl;
  std::cout << "Uncategorized file type: '" << ext
            << "' for file: " << file.filename().string() << std::endl;
  std::cout << "Where should files of this type go?" << std::endl;
  std::cout << "  1. Put in 'Other' folder" << std::endl;
  std::cout << "  2. Create a new folder" << std::endl;
  std::cout << "Enter your choice (1-2): ";

  int choice = 0;
  std::cin >> choice;
  while (std::cin.fail() || (choice != 1 && choice != 2)) {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Invalid input. Please enter 1 or 2: ";
    std::cin >> choice;
  }

  fs::path targetSubDir;
  if (choice == 2) {
    std::cout << "Enter new folder name (e.g., 'CAD_Files'): ";
    std::string newDirName;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, newDirName);
    targetSubDir = newDirName;
  } else {
    targetSubDir = "Other";
  }

  std::cout << "'" << ext << "' files will now be placed in '"
            << targetSubDir.string() << "'." << std::endl;
  std::cout << "--------------------------------------------------\n"
            << std::endl;

  userRules[ext] = targetSubDir;
  return destBaseDir / targetSubDir;
}
