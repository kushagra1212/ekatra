# Ekatra

A command-line tool to merge and organize messy folders.

Ekatra cleans up scattered files by taking two directories, merging their contents, and sorting everything into a clean folder structure. It's a simple C++ tool for a common problem.

## What It Does

- **Sorts files by type:** Automatically puts files into folders like `Media/Images`, `Documents/Text`, `Archives`, etc.

- **Copies or Moves files:** You can choose to `copy` files (safe) or `move` them (faster).

- **Handles duplicate filenames:** By default, it renames duplicates (`file_1.txt`) so nothing is overwritten. You can also tell it to just skip them.

- **Asks about unknown files:** If it finds a file type it doesn't recognize, it asks you where to put it and remembers your choice.

- **Works anywhere:** It's standard C++17, so it compiles and runs on macOS, Linux, and Windows.

## The Folder Structure It Creates

Here’s where your files will end up:

```
📁 Destination/
├── 📁 Media/
│   ├── 📁 Images/ (.jpg, .png, .gif, ...)
│   └── 📁 Videos/ (.mp4, .mov, .avi, ...)
├── 📁 Documents/
│   ├── 📁 Text/ (.pdf, .doc, .docx, .txt, ...)
│   ├── 📁 Spreadsheets/ (.xls, .xlsx, .csv, ...)
│   └── 📁 Presentations/ (.ppt, .pptx, .key, ...)
├── 📁 Audio/ (.mp3, .wav, .aac, ...)
├── 📁 Archives/ (.zip, .rar, .7z, ...)
├── 📁 Code/ (.cpp, .py, .js, ...)
└── 📁 Other/ (For anything it doesn't recognize)
```

## How to Use It

Run it from your terminal like this:

```bash
ekatra <source_A> <source_B> <destination> [options]

```

### Arguments & Options

| Argument      | Description                                        |
| ------------- | -------------------------------------------------- |
| `source_A`    | **Required.** The first folder.                    |
| `source_B`    | **Required.** The second folder.                   |
| `destination` | **Required.** Where the organized files should go. |

| Option              | Shorthand | Description                                              | Default |
| ------------------- | --------- | -------------------------------------------------------- | ------- |
| `--mode <mode>`     |           | Use `copy` or `move`.                                    | `copy`  |
| `--skip-duplicates` |           | Skips files that already exist instead of renaming them. | `false` |
| `--verbose`         | `-v`      | Shows every file being processed.                        | `false` |
| `--help`            |           | Shows the help message.                                  |         |

### Examples

**Basic copy:**

```bash
./ekatra ~/Downloads/FolderA ~/Desktop/FolderB ~/Documents/Merged --verbose
```

**Move files and skip any duplicates:**

```bash
/ekatra /Volumes/External/Photos /Volumes/Backup/OldPhotos ~/Pictures/Organized --mode move --skip-duplicates
```

## Building from Source

### You'll need:

- A C++17 compiler (Clang, GCC, MSVC)

- CMake (3.16 or newer)

### Steps:

1. **Clone the repo:**

```bash
git clone https://github.com/your-username/ekatra.git
cd ekatra
```

2. **Set up the build directory with CMake:**

```bash
cmake -S . -B build
```

3. **Compile it:**

```bash
cmake --build build
```

4. **Run it:**
   The program will be in the `build` folder

```bash
./build/ekatra --help
```

Copyright (c) 2025 Kushagra Rathore. Released under the GNU GPL v2.0 License.
