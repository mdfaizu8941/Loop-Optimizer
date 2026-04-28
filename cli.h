#pragma once

#include <string>

struct Args {
    std::string input;
    bool hasCompiler = false;
    std::string compiler;
    std::string output = "ouput.c";
    bool hasReportJson = false;
    std::string reportJson;
    bool noCompare = false;
    bool keepBinaries = false;
    bool failOnError = false;
};

bool parseArgs(int argc, char **argv, Args &args);
void printUsage(const char *prog);
