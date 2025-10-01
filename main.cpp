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

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  fs::path sourceA = program.get<std::string>("source_a");
  fs::path sourceB = program.get<std::string>("source_b");
  fs::path dest = program.get<std::string>("destination");
  auto modeStr = program.get<std::string>("--mode");
  bool verbose = program.get<bool>("--verbose");

  MergeManager::Operation op = MergeManager::Operation::Copy;
  if (modeStr == "move") {
    op = MergeManager::Operation::Move;
    std::cout << "⚠️ Running in MOVE mode. Original files will be deleted."
              << std::endl;
  } else {
    std::cout << "Running in COPY mode. Original files will be preserved."
              << std::endl;
  }

  MergeManager manager;
  manager.process(sourceA, sourceB, dest, op, verbose);

  return 0;
}