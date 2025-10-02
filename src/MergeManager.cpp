#include "MergeManager.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <string>

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

fs::path MergeManager::getDestinationForFile(const fs::path &file,
                                             const fs::path &destBaseDir) {
  std::string ext = file.extension().string();
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

fs::path MergeManager::handleUnknownFile(const fs::path &file,
                                         const fs::path &destBaseDir) {
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

  fs::path targetSubDir;

  if (choice == 2) {
    std::cout << "Enter new folder name (e.g., 'CAD_Files'): ";
    std::string newDirName;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),
                    '\n'); // Safely clear buffer
    std::getline(std::cin, newDirName);
    targetSubDir = newDirName;
  } else {
    targetSubDir = "Other";
  }

  std::cout << "'" << ext << "' files will now be placed in '"
            << targetSubDir.string() << "'." << std::endl;
  std::cout << "--------------------------------------------------\n"
            << std::endl;

  // Save this new rule for the rest of the session
  m_userRules[ext] = targetSubDir;
  return destBaseDir / targetSubDir;
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

void MergeManager::processDirectory(const fs::path &sourceDir,
                                    const fs::path &destBaseDir, Operation op,
                                    bool verbose, bool skipDuplicates) {
  try {
    for (const auto &entry : fs::recursive_directory_iterator(sourceDir)) {
      if (!fs::is_regular_file(entry.symlink_status()))
        continue;

      if (verbose) {
        std::cout << "Processing: " << entry.path().string() << std::endl;
      }

      fs::path targetDir = getDestinationForFile(entry.path(), destBaseDir);

      if (targetDir.empty()) {
        targetDir = handleUnknownFile(entry.path(), destBaseDir);
      }

      fs::create_directories(targetDir);

      fs::path destFile;
      if (skipDuplicates) {
        destFile = targetDir / entry.path().filename();
        if (fs::exists(destFile)) {
          if (verbose) {
            std::cout << "  -> Skipping (already exists): "
                      << destFile.filename().string() << std::endl;
          }
          continue;
        }
      } else {
        destFile = getUniquePath(targetDir / entry.path().filename());
      }

      std::error_code ec;
      if (op == Operation::Copy) {
        fs::copy(entry.path(), destFile, ec);
      } else {
        fs::rename(entry.path(), destFile, ec);
      }

      if (ec) {
        std::cerr << "Error processing " << entry.path().string() << ": "
                  << ec.message() << std::endl;
      }
    }
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
  }
}

void MergeManager::process(const fs::path &sourceA, const fs::path &sourceB,
                           const fs::path &dest, Operation op, bool verbose,
                           bool skipDuplicates) {
  if (!fs::exists(sourceA) || !fs::exists(sourceB)) {
    std::cerr << "Error: One or both source folders do not exist." << std::endl;
    return;
  }

  try {
    fs::create_directories(dest);
    std::cout << "Starting merge for Folder A: " << sourceA.string()
              << std::endl;
    processDirectory(sourceA, dest, op, verbose, skipDuplicates);

    std::cout << "Starting merge for Folder B: " << sourceB.string()
              << std::endl;
    processDirectory(sourceB, dest, op, verbose, skipDuplicates);

    std::cout << "\n✅ Merge operation completed successfully!" << std::endl;
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
  }
}