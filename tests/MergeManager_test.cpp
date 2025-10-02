#include "../src/MergeManager.h"
#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

// Test fixture for MergeManager tests.
// This sets up a clean, temporary directory structure for each test.
class MergeManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a unique temporary directory for the test run.
    baseDir = fs::path(testing::TempDir()) / "EkatraTest";
    fs::create_directories(baseDir);

    // Set up our standard source and destination folders inside the temp dir.
    sourceA = baseDir / "src_a";
    sourceB = baseDir / "src_b";
    dest = baseDir / "dest";
    fs::create_directories(sourceA);
    fs::create_directories(sourceB);
    fs::create_directories(dest);
  }

  void TearDown() override {
    // Clean up the temporary directory after each test.
    // This is good practice to ensure a clean state.
    std::error_code ec;
    fs::remove_all(baseDir, ec);
    if (ec) {
      // Handle or log error if necessary
    }
  }

  // Helper function to easily create dummy files for tests.
  void createFile(const fs::path &path, bool empty = false) {
    fs::create_directories(path.parent_path());
    std::ofstream ofs(path);
    if (!empty) {
      ofs << "test";
    }
    ofs.close();
  }

  MergeManager manager;
  fs::path baseDir;
  fs::path sourceA;
  fs::path sourceB;
  fs::path dest;
};

// --- Tests for getDestinationForFile ---

TEST_F(MergeManagerTest, GetDestinationForFile_KnownExtension) {
  fs::path result = manager.getDestinationForFile("photo.JPG", dest);
  ASSERT_EQ(result, dest / "Media/Images");
}

TEST_F(MergeManagerTest, GetDestinationForFile_UnknownExtension) {
  fs::path result = manager.getDestinationForFile("archive.dat", dest);
  // An empty path signals that the file type is unknown.
  ASSERT_TRUE(result.empty());
}

TEST_F(MergeManagerTest, GetDestinationForFile_NoExtension) {
  fs::path result = manager.getDestinationForFile("README", dest);
  ASSERT_TRUE(result.empty());
}

TEST_F(MergeManagerTest, GetDestinationForFile_DotFile) {
  fs::path result = manager.getDestinationForFile(".config", dest);
  ASSERT_TRUE(result.empty());
}

// --- Tests for the main 'process' logic ---

TEST_F(MergeManagerTest, Process_BasicCopy) {
  createFile(sourceA / "report.pdf");
  createFile(sourceB / "image.png");

  manager.process(sourceA, sourceB, dest, MergeManager::Operation::Copy, false,
                  false);

  // Verify files are sorted correctly in the destination
  ASSERT_TRUE(fs::exists(dest / "Documents/Text/report.pdf"));
  ASSERT_TRUE(fs::exists(dest / "Media/Images/image.png"));

  // Verify original files still exist
  ASSERT_TRUE(fs::exists(sourceA / "report.pdf"));
  ASSERT_TRUE(fs::exists(sourceB / "image.png"));
}

TEST_F(MergeManagerTest, Process_BasicMove) {
  createFile(sourceA / "archive.zip");
  createFile(sourceB / "video.mp4");

  manager.process(sourceA, sourceB, dest, MergeManager::Operation::Move, false,
                  false);

  // Verify files are sorted correctly in the destination
  ASSERT_TRUE(fs::exists(dest / "Archives/archive.zip"));
  ASSERT_TRUE(fs::exists(dest / "Media/Videos/video.mp4"));

  // Verify original files are gone
  ASSERT_FALSE(fs::exists(sourceA / "archive.zip"));
  ASSERT_FALSE(fs::exists(sourceB / "video.mp4"));
}

TEST_F(MergeManagerTest, Process_DuplicateFilenameRenaming) {
  createFile(sourceA / "duplicate.txt");
  createFile(sourceB / "duplicate.txt");

  manager.process(sourceA, sourceB, dest, MergeManager::Operation::Copy, false,
                  false);

  // Verify both files exist, with one renamed
  ASSERT_TRUE(fs::exists(dest / "Documents/Text/duplicate.txt"));
  ASSERT_TRUE(fs::exists(dest / "Documents/Text/duplicate_1.txt"));
}

TEST_F(MergeManagerTest, Process_DuplicateFilenameSkipping) {
  createFile(sourceA / "duplicate.txt");
  createFile(sourceB / "duplicate.txt");

  // Run with skipDuplicates = true
  manager.process(sourceA, sourceB, dest, MergeManager::Operation::Copy, false,
                  true);

  // Verify only one file exists and the renamed one does not
  ASSERT_TRUE(fs::exists(dest / "Documents/Text/duplicate.txt"));
  ASSERT_FALSE(fs::exists(dest / "Documents/Text/duplicate_1.txt"));
}

TEST_F(MergeManagerTest, Process_HandlesNestedDirectories) {
  createFile(sourceA / "deep" / "nested" / "folder" / "code.py");

  manager.process(sourceA, sourceB, dest, MergeManager::Operation::Copy, false,
                  false);

  // Verify the file is moved to the correct category, flattening the structure
  ASSERT_TRUE(fs::exists(dest / "Code/code.py"));
  ASSERT_FALSE(fs::exists(dest / "deep")); // The nested structure is not copied
}

TEST_F(MergeManagerTest, Process_HandlesEmptySourceDirectory) {
  createFile(sourceB / "audio.mp3");

  // sourceA is empty
  manager.process(sourceA, sourceB, dest, MergeManager::Operation::Copy, false,
                  false);

  // Verify the file from sourceB was processed correctly and no errors occurred
  ASSERT_TRUE(fs::exists(dest / "Audio/audio.mp3"));
  ASSERT_FALSE(fs::exists(dest / "Documents")); // No other folders created
}

TEST_F(MergeManagerTest, Process_HandlesFilenamesWithSpaces) {
  createFile(sourceA / "My Important Presentation.pptx");

  manager.process(sourceA, sourceB, dest, MergeManager::Operation::Copy, false,
                  false);

  ASSERT_TRUE(fs::exists(
      dest / "Documents/Presentations/My Important Presentation.pptx"));
}

TEST_F(MergeManagerTest, Process_HandlesSourceAndDestinationOverlap) {
  createFile(sourceA / "presentation.key");
  // Here, one of the source folders IS the destination.
  manager.process(sourceA, sourceB, sourceA, MergeManager::Operation::Move,
                  false, false);

  // The file should be moved into a categorized subdirectory within the source
  // folder. This ensures the tool doesn't enter an infinite loop or corrupt
  // data.
  ASSERT_TRUE(fs::exists(sourceA / "Documents/Presentations/presentation.key"));

  // The original file at the root of the source folder should be gone.
  ASSERT_FALSE(fs::exists(sourceA / "presentation.key"));
}

TEST_F(MergeManagerTest, Process_HandlesSpecialCharactersInFilenames) {
  std::string specialName = "file-with-!@#$&-éà-你好.txt";
  createFile(sourceA / specialName);
  manager.process(sourceA, sourceB, dest, MergeManager::Operation::Copy, false,
                  false);
  ASSERT_TRUE(fs::exists(dest / "Documents/Text" / specialName));
}

TEST_F(MergeManagerTest, Process_HandlesEmptyFiles) {
  createFile(sourceA / "empty.txt", true); // Create a zero-byte file
  manager.process(sourceA, sourceB, dest, MergeManager::Operation::Copy, false,
                  false);
  ASSERT_TRUE(fs::exists(dest / "Documents/Text/empty.txt"));
  ASSERT_EQ(fs::file_size(dest / "Documents/Text/empty.txt"), 0);
}

TEST_F(MergeManagerTest, Process_HandlesSubdirectoriesNamedLikeCategories) {
  createFile(sourceA / "Media" / "my-song.mp3");
  manager.process(sourceA, sourceB, dest, MergeManager::Operation::Copy, false,
                  false);
  // Should not create dest/Media/Media/my-song.mp3
  ASSERT_TRUE(fs::exists(dest / "Audio/my-song.mp3"));
}

TEST_F(MergeManagerTest, Process_HandlesCaseVariantDuplicateFilenames) {
  createFile(sourceA / "report.pdf");
  createFile(sourceB / "REPORT.PDF");
  manager.process(sourceA, sourceB, dest, MergeManager::Operation::Copy, false,
                  false);

  // This behavior is filesystem-dependent. On case-insensitive systems
  // (Windows, macOS), this should result in one file and one renamed file. On
  // case-sensitive (Linux), it would result in two separate files. This test
  // checks the rename behavior.
  bool firstExists = fs::exists(dest / "Documents/Text/report.pdf");
  bool secondExists = fs::exists(dest / "Documents/Text/REPORT.PDF");
  bool renamedExists = fs::exists(dest / "Documents/Text/report_1.pdf") ||
                       fs::exists(dest / "Documents/Text/REPORT_1.PDF");

  // On any system, we expect two files in the end.
  // This could be (report.pdf, REPORT.PDF) or (report.pdf, report_1.pdf).
  // We check that some combination of two files exists.
  ASSERT_TRUE((firstExists && secondExists) || (firstExists && renamedExists));
}

#ifndef _WIN32
TEST_F(MergeManagerTest, Process_SkipsSymbolicLinks) {
  fs::path targetFile = sourceA / "target.txt";
  createFile(targetFile);
  fs::path symlink = sourceA / "link.txt";
  fs::create_symlink(targetFile, symlink);

  manager.process(sourceA, sourceB, dest, MergeManager::Operation::Copy, false,
                  false);

  // Verify the real file was copied
  ASSERT_TRUE(fs::exists(dest / "Documents/Text/target.txt"));
  // Verify the symbolic link was NOT copied
  ASSERT_FALSE(fs::exists(dest / "Documents/Text/link.txt"));
}
#endif

TEST_F(MergeManagerTest, Process_HandlesIdenticalSourceFolders) {
  createFile(sourceA / "unique.txt");
  // Both sourceA and sourceB point to the same directory
  manager.process(sourceA, sourceA, dest, MergeManager::Operation::Copy, false,
                  false);

  // The directory is processed twice, so we expect the original and a renamed
  // copy.
  ASSERT_TRUE(fs::exists(dest / "Documents/Text/unique.txt"));
  ASSERT_TRUE(fs::exists(dest / "Documents/Text/unique_1.txt"));
}