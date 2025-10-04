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
    options.sourceA = baseDir / "src_a";
    options.sourceB = baseDir / "src_b";
    options.destination = baseDir / "dest";
    options.skipDuplicates = false;
    options.operation = MergeManager::Operation::Copy;
    options.verbose = false;
    options.includeHidden = false;

    fs::create_directories(options.sourceA);
    fs::create_directories(options.sourceB);
    fs::create_directories(options.destination);
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

  std::vector<std::string> readLines(const fs::path &path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
      lines.push_back(line);
    }
    return lines;
  }

  MergeManager manager;
  fs::path baseDir;
  ProcessOptions options;
};

// --- Tests for getDestinationForFile ---

TEST_F(MergeManagerTest, GetDestinationForFile_KnownExtension) {
  fs::path result =
      manager.getDestinationForFile("photo.JPG", options.destination);
  ASSERT_EQ(result, options.destination / "Media/Images");
}

TEST_F(MergeManagerTest, GetDestinationForFile_UnknownExtension) {
  fs::path result =
      manager.getDestinationForFile("archive.dat", options.destination);
  // An empty path signals that the file type is unknown.
  ASSERT_TRUE(result.empty());
}

TEST_F(MergeManagerTest, GetDestinationForFile_NoExtension) {
  fs::path result =
      manager.getDestinationForFile("README", options.destination);
  ASSERT_TRUE(result.empty());
}

TEST_F(MergeManagerTest, GetDestinationForFile_DotFile) {
  fs::path result =
      manager.getDestinationForFile(".config", options.destination);
  ASSERT_TRUE(result.empty());
}

// --- Tests for the main 'process' logic ---

TEST_F(MergeManagerTest, Process_BasicCopy) {
  createFile(options.sourceA / "report.pdf");
  createFile(options.sourceB / "image.png");

  options.operation = MergeManager::Operation::Copy;
  manager.process(options);

  // Verify files are sorted correctly in the destination
  ASSERT_TRUE(fs::exists(options.destination / "Documents/Text/report.pdf"));
  ASSERT_TRUE(fs::exists(options.destination / "Media/Images/image.png"));

  // Verify original files still exist
  ASSERT_TRUE(fs::exists(options.sourceA / "report.pdf"));
  ASSERT_TRUE(fs::exists(options.sourceB / "image.png"));
}

TEST_F(MergeManagerTest, Process_BasicMove) {
  createFile(options.sourceA / "archive.zip");
  createFile(options.sourceB / "video.mp4");

  options.operation = MergeManager::Operation::Move;
  manager.process(options);

  // Verify files are sorted correctly in the destination
  ASSERT_TRUE(fs::exists(options.destination / "Archives/archive.zip"));
  ASSERT_TRUE(fs::exists(options.destination / "Media/Videos/video.mp4"));

  // Verify original files are gone
  ASSERT_FALSE(fs::exists(options.sourceA / "archive.zip"));
  ASSERT_FALSE(fs::exists(options.sourceB / "video.mp4"));
}

TEST_F(MergeManagerTest, Process_DuplicateFilenameRenaming) {
  createFile(options.sourceA / "duplicate.txt");
  createFile(options.sourceB / "duplicate.txt");

  options.skipDuplicates = false;
  manager.process(options);

  // Verify both files exist, with one renamed
  ASSERT_TRUE(fs::exists(options.destination / "Documents/Text/duplicate.txt"));
  ASSERT_TRUE(
      fs::exists(options.destination / "Documents/Text/duplicate_1.txt"));
}

TEST_F(MergeManagerTest, Process_DuplicateFilenameSkipping) {
  createFile(options.sourceA / "duplicate.txt");
  createFile(options.sourceB / "duplicate.txt");

  options.skipDuplicates = true;
  manager.process(options);

  manager.process(options);

  // Verify only one file exists and the renamed one does not
  ASSERT_TRUE(fs::exists(options.destination / "Documents/Text/duplicate.txt"));
  ASSERT_FALSE(
      fs::exists(options.destination / "Documents/Text/duplicate_1.txt"));
}

TEST_F(MergeManagerTest, Process_HandlesNestedDirectories) {
  createFile(options.sourceA / "deep" / "nested" / "folder" / "code.py");

  manager.process(options);

  // Verify the file is moved to the correct category, flattening the structure
  ASSERT_TRUE(fs::exists(options.destination / "Code/code.py"));
  ASSERT_FALSE(fs::exists(options.destination /
                          "deep")); // The nested structure is not copied
}

TEST_F(MergeManagerTest, Process_HandlesEmptySourceDirectory) {
  createFile(options.sourceB / "audio.mp3");

  // options.sourceA is empty
  manager.process(options);

  // Verify the file from options.sourceB was processed correctly and no errors
  // occurred
  ASSERT_TRUE(fs::exists(options.destination / "Audio/audio.mp3"));
  ASSERT_FALSE(fs::exists(options.destination /
                          "Documents")); // No other folders created
}

TEST_F(MergeManagerTest, Process_HandlesFilenamesWithSpaces) {
  createFile(options.sourceA / "My Important Presentation.pptx");

  manager.process(options);

  ASSERT_TRUE(
      fs::exists(options.destination /
                 "Documents/Presentations/My Important Presentation.pptx"));
}

TEST_F(MergeManagerTest, Process_HandlesSourceAndDestinationOverlap) {
  createFile(options.sourceA / "presentation.key");
  // Here, one of the source folders IS the destination.
  options.destination = options.sourceA;
  options.operation = MergeManager::Operation::Move;
  manager.process(options);

  // The file should be moved into a categorized subdirectory within the source
  // folder. This ensures the tool doesn't enter an infinite loop or corrupt
  // data.
  ASSERT_TRUE(
      fs::exists(options.sourceA / "Documents/Presentations/presentation.key"));

  // The original file at the root of the source folder should be gone.
  ASSERT_FALSE(fs::exists(options.sourceA / "presentation.key"));
}

TEST_F(MergeManagerTest, Process_HandlesSpecialCharactersInFilenames) {
  std::string specialName = "file-with-!@#$&-éà-你好.txt";
  createFile(options.sourceA / specialName);

  manager.process(options);
  ASSERT_TRUE(fs::exists(options.destination / "Documents/Text" / specialName));
}

TEST_F(MergeManagerTest, Process_HandlesEmptyFiles) {
  createFile(options.sourceA / "empty.txt", true); // Create a zero-byte file

  manager.process(options);
  ASSERT_TRUE(fs::exists(options.destination / "Documents/Text/empty.txt"));
  ASSERT_EQ(fs::file_size(options.destination / "Documents/Text/empty.txt"), 0);
}

TEST_F(MergeManagerTest, Process_HandlesSubdirectoriesNamedLikeCategories) {
  createFile(options.sourceA / "Media" / "my-song.mp3");

  manager.process(options);
  // Should not create options.destination /Media/Media/my-song.mp3
  ASSERT_TRUE(fs::exists(options.destination / "Audio/my-song.mp3"));
}

TEST_F(MergeManagerTest, Process_HandlesCaseVariantDuplicateFilenames) {
  createFile(options.sourceA / "report.pdf");
  createFile(options.sourceB / "REPORT.PDF");

  manager.process(options);

  // This behavior is filesystem-dependent. On case-insensitive systems
  // (Windows, macOS), this should result in one file and one renamed file. On
  // case-sensitive (Linux), it would result in two separate files. This test
  // checks the rename behavior.
  bool firstExists =
      fs::exists(options.destination / "Documents/Text/report.pdf");
  bool secondExists =
      fs::exists(options.destination / "Documents/Text/REPORT.PDF");
  bool renamedExists =
      fs::exists(options.destination / "Documents/Text/report_1.pdf") ||
      fs::exists(options.destination / "Documents/Text/REPORT_1.PDF");

  // On any system, we expect two files in the end.
  // This could be (report.pdf, REPORT.PDF) or (report.pdf, report_1.pdf).
  // We check that some combination of two files exists.
  ASSERT_TRUE((firstExists && secondExists) || (firstExists && renamedExists));
}

#ifndef _WIN32
TEST_F(MergeManagerTest, Process_SkipsSymbolicLinks) {
  fs::path targetFile = options.sourceA / "target.txt";
  createFile(targetFile);
  fs::path symlink = options.sourceA / "link.txt";
  fs::create_symlink(targetFile, symlink);

  manager.process(options);

  // Verify the real file was copied
  ASSERT_TRUE(fs::exists(options.destination / "Documents/Text/target.txt"));
  // Verify the symbolic link was NOT copied
  ASSERT_FALSE(fs::exists(options.destination / "Documents/Text/link.txt"));
}
#endif

TEST_F(MergeManagerTest, Process_HandlesIdenticalSourceFolders) {
  createFile(options.sourceA / "unique.txt");
  // Both options.sourceA and options.sourceB point to the same directory

  options.sourceB = options.sourceA;
  manager.process(options);

  // The directory is processed twice, so we expect the original and a renamed
  // copy.
  ASSERT_TRUE(fs::exists(options.destination / "Documents/Text/unique.txt"));
  ASSERT_TRUE(fs::exists(options.destination / "Documents/Text/unique_1.txt"));
}

TEST_F(MergeManagerTest, Process_IgnoresHiddenFilesByDefault) {
  createFile(options.sourceA / "normal.txt");
  createFile(options.sourceB / ".hidden_file");

  // Default options have includeHidden = false
  manager.process(options);

  // Verify the normal file was copied
  ASSERT_TRUE(fs::exists(options.destination / "Documents/Text/normal.txt"));
  // Verify the hidden file was ignored
  ASSERT_FALSE(fs::exists(options.destination / "Other/.hidden_file"));
}

TEST_F(MergeManagerTest, Process_IncludesHiddenFilesWhenFlagged) {
  createFile(options.sourceA / "normal.txt");
  createFile(options.sourceB / ".hidden_file");

  // Enable the option to include hidden files
  options.includeHidden = true;

  std::string simulated_input = "1\n"; // Choose "Other" for unknown file types
  std::stringstream input_stream(simulated_input);

  auto *cin_orig = std::cin.rdbuf();
  std::cin.rdbuf(input_stream.rdbuf());

  manager.process(options);

  // restore cin
  std::cin.rdbuf(cin_orig);

  // Verify both files were copied
  ASSERT_TRUE(fs::exists(options.destination / "Documents/Text/normal.txt"));
  ASSERT_TRUE(fs::exists(options.destination / "Other/.hidden_file"));
}

TEST_F(MergeManagerTest, Process_IncludesHiddenFilesWhenFlaggedPutInNewFolder) {
  createFile(options.sourceA / "normal.txt");
  createFile(options.sourceB / ".hidden_file");

  // Enable the option to include hidden files
  options.includeHidden = true;

  std::string simulated_input = "2\nMyHiddenFiles\n";
  std::stringstream input_stream(simulated_input);

  auto *cin_orig = std::cin.rdbuf();
  std::cin.rdbuf(input_stream.rdbuf());

  manager.process(options);

  // restore cin
  std::cin.rdbuf(cin_orig);

  // Verify both files were copied
  ASSERT_TRUE(fs::exists(options.destination / "Documents/Text/normal.txt"));
  ASSERT_TRUE(fs::exists(options.destination / "MyHiddenFiles/.hidden_file"));
}

TEST_F(MergeManagerTest,
       Process_IncludesRegexHiddenFilesWhenFlaggedPutInNewFolder) {
  createFile(options.sourceA / "normal.txt");
  createFile(options.sourceB / ".hidden_file-2025.log");

  // Enable the option to include hidden files
  options.includeHidden = true;

  std::string simulated_input = "3\n^\\.hidden_file-.*\\.log$\nMyHiddenFiles\n";
  std::stringstream input_stream(simulated_input);

  auto *cin_orig = std::cin.rdbuf();
  std::cin.rdbuf(input_stream.rdbuf());

  manager.process(options);

  // restore cin
  std::cin.rdbuf(cin_orig);

  // Verify both files were copied
  ASSERT_TRUE(fs::exists(options.destination / "Documents/Text/normal.txt"));
  ASSERT_TRUE(
      fs::exists(options.destination / "MyHiddenFiles/.hidden_file-2025.log"));
}

TEST_F(MergeManagerTest, Process_SortsFilesWithCustomRegexRules) {
  // 1. Create a temporary rules file
  fs::path rulesFilePath = baseDir / "custom_rules.txt";
  std::ofstream rulesFile(rulesFilePath);
  rulesFile << "# Custom rules for invoices and receipts\n";
  rulesFile << "^invoice-.*\\.pdf$:Financial/Invoices\n";
  rulesFile << ".*-receipt\\.jpg$:Financial/Receipts\n";
  rulesFile.close();

  // 2. Create files that match the rules, and one that doesn't
  createFile(options.sourceA / "invoice-2025-01.pdf");
  createFile(options.sourceB / "store-receipt.jpg");
  createFile(options.sourceA / "regular-photo.png");

  // 3. Set the rules file option and run the process
  options.rulesFile = rulesFilePath.string();
  manager.process(options);

  // 4. Verify files are sorted according to the custom rules
  ASSERT_TRUE(fs::exists(options.destination /
                         "Financial/Invoices/invoice-2025-01.pdf"));
  ASSERT_TRUE(
      fs::exists(options.destination / "Financial/Receipts/store-receipt.jpg"));
  // Verify the non-matching file was sorted by the default rules
  ASSERT_TRUE(
      fs::exists(options.destination / "Media/Images/regular-photo.png"));
}

TEST_F(MergeManagerTest, Process_InteractiveRegexRuleCreation) {
  // 1. SETUP: Create a file with an unknown extension that will trigger the
  // prompt.
  createFile(options.sourceA / "project-alpha-report.dat");

  // 2. PREPARE INPUT: This string simulates exactly what a user would type.
  //    - "3" to select the regex option.
  //    - The regex rule, followed by a newline.
  //    - The destination folder, followed by a newline.
  std::string simulated_input =
      "3\n^project-alpha-.*\\.dat$\nReports/ProjectAlpha\n";
  std::stringstream input_stream(simulated_input);

  // 3. REDIRECT CIN: Save the original cin buffer and redirect it to our
  // stream.
  auto *cin_orig = std::cin.rdbuf();
  std::cin.rdbuf(input_stream.rdbuf());

  // 4. RUN THE PROCESS: The manager.process() call will now read from our
  // simulated input.
  manager.process(options);

  // 5. RESTORE CIN: ALWAYS restore the original cin buffer to not affect other
  // tests.
  std::cin.rdbuf(cin_orig);

  // 6. VERIFY THE OUTCOME: Check if the file was moved to the new folder
  // defined by the user interactively.
  ASSERT_TRUE(fs::exists(options.destination /
                         "Reports/ProjectAlpha/project-alpha-report.dat"));
}

TEST_F(MergeManagerTest, ScanOnly_IdentifiesUncategorizedFiles) {
  // 1. SETUP: Create a mix of known and unknown files.
  fs::path knownFile = options.sourceA / "known.txt";
  fs::path unknownFile1 = options.sourceB / "unknown.dat";
  fs::path unknownFile2 = options.sourceA / "archive.special";
  createFile(knownFile);
  createFile(unknownFile1);
  createFile(unknownFile2);

  // 2. ACTION: Run the scan and specify an output file.
  fs::path scanOutputPath = baseDir / "scan_results.txt";
  options.scanFile = scanOutputPath.string();
  manager.scanOnly(options);

  // 3. VERIFY:
  // Check that the output file was created.
  ASSERT_TRUE(fs::exists(scanOutputPath));

  // Read the file and check its contents.
  std::vector<std::string> lines = readLines(scanOutputPath);
  ASSERT_EQ(lines.size(), 2); // Should only contain the 2 unknown files.

  // Check that the paths of the unknown files are in the output.
  // We use find because the order isn't guaranteed.
  ASSERT_NE(std::find(lines.begin(), lines.end(), unknownFile1.string()),
            lines.end());
  ASSERT_NE(std::find(lines.begin(), lines.end(), unknownFile2.string()),
            lines.end());

  // Check that the known file is NOT in the output.
  ASSERT_EQ(std::find(lines.begin(), lines.end(), knownFile.string()),
            lines.end());

  // Check that no files were actually moved or copied.
  ASSERT_TRUE(fs::is_empty(options.destination));
}

TEST_F(MergeManagerTest, ScanOnly_HandlesNoUncategorizedFiles) {
  // 1. SETUP: Create only files with known extensions.
  createFile(options.sourceA / "document.pdf");
  createFile(options.sourceB / "photo.jpg");

  // 2. ACTION: Run the scan.
  fs::path scanOutputPath = baseDir / "scan_results.txt";
  options.scanFile = scanOutputPath.string();
  manager.scanOnly(options);

  // 3. VERIFY:
  // Check that the output file was NOT created, because there was nothing to
  // report.
  ASSERT_FALSE(fs::exists(scanOutputPath));

  // Check that no files were actually moved or copied.
  ASSERT_TRUE(fs::is_empty(options.destination));
}

TEST_F(MergeManagerTest, Process_SimpleMergeNoSort) {
  // 1. SETUP: Create some files, including a duplicate.
  createFile(options.sourceA / "file1.txt");
  createFile(options.sourceA / "duplicate.log");
  createFile(options.sourceA / "docs/document.pdf");
  createFile(options.sourceA / "media/picture1.png");
  createFile(options.sourceA / "media/5.png");
  createFile(options.sourceB / "file2.jpg");
  createFile(options.sourceB / "duplicate.log");
  createFile(options.sourceB / "media/picture2.png");
  createFile(options.sourceB / "media/5.png");

  // 2. ACTION: Run the process with the no-sort flag enabled.
  options.noSort = true;
  manager.process(options);

  // 3. VERIFY:
  // Check that the unique files were copied directly into the destination.
  ASSERT_TRUE(fs::exists(options.destination / "file1.txt"));
  ASSERT_TRUE(fs::exists(options.destination / "file2.jpg"));
  ASSERT_TRUE(fs::exists(options.destination / "docs/document.pdf"));
  ASSERT_TRUE(fs::exists(options.destination / "media/picture1.png"));
  ASSERT_TRUE(fs::exists(options.destination / "media/picture2.png"));

  // Check that the duplicate file exists (only one copy).
  ASSERT_TRUE(fs::exists(options.destination / "duplicate.log"));
  ASSERT_FALSE(fs::exists(options.destination / "duplicate_1.log"));
  ASSERT_FALSE(fs::exists(options.destination / "media/5_1.png"));
}