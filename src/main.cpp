#include "MergeManager.h"
#include "include/argparse/argparse.hpp"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {

  argparse::ArgumentParser program("Ekatra", "1.0");

  program.add_argument("source_a").help("First source folder to merge.");

  program.add_argument("source_b").help("Second source folder to merge.");

  program.add_argument("destination")
      .help("Destination folder where merged files will be organized.");

  program.add_argument("--mode")
      .help("Operation mode: 'copy' (default) or 'move'.")
      .default_value(std::string("copy"));

  program.add_argument("-v", "--verbose")
      .help("Enable verbose output to see every file being processed.")
      .default_value(false)
      .implicit_value(true);

  program.add_argument("-s", "--skip-duplicates")
      .help("If a file with the same name already exists, skip it instead of "
            "creating a renamed copy.")
      .default_value(false)
      .implicit_value(true);

  program.add_argument("--include-hidden")
      .help("Include hidden files (files starting with a dot). Ignored by "
            "default.")
      .default_value(false)
      .implicit_value(true);

  program.add_argument("--rules")
      .help("Path to a text file containing custom regex sorting rules.")
      .default_value(std::string(""));
  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }
  ProcessOptions options;
  options.sourceA = program.get<std::string>("source_a");
  options.sourceB = program.get<std::string>("source_b");
  options.destination = program.get<std::string>("destination");
  options.verbose = program.get<bool>("--verbose");
  options.skipDuplicates = program.get<bool>("--skip-duplicates");
  options.includeHidden = program.get<bool>("--include-hidden");
  options.rulesFile = program.get<std::string>("--rules");

  if (program.get<std::string>("--mode") == "move") {
    options.operation = MergeManager::Operation::Move;
    std::cout << "Running in MOVE mode. Original files will be deleted."
              << std::endl;
  } else {
    options.operation = MergeManager::Operation::Copy;
    std::cout << "Running in COPY mode. Original files will be preserved."
              << std::endl;
  }

  MergeManager manager;
  manager.process(options);

  return 0;
}