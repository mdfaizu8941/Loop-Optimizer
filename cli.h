#pragma once

#include <string>

struct Args {
    std::string input;
    bool hasCompiler = false;
    std::string compiler;
    bool noCompare = false;
    bool keepBinaries = false;
};

bool parseArgs(int argc, char **argv, Args &args);
void printUsage(const char *prog);
