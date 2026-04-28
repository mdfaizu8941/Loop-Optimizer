#include "cli.h"

#include <iostream>
#include <string>

using namespace std;

bool parseArgs(int argc, char **argv, Args &args) {
    if (argc < 2) {
        return false;
    }

    args.input = argv[1];

    for (int i = 2; i < argc; ++i) {
        const string token = argv[i];
        if (token == "--no-compare") {
            args.noCompare = true;
        } else if (token == "--keep-binaries") {
            args.keepBinaries = true;
        } else if (token == "--fail-on-error") {
            args.failOnError = true;
        } else if (token == "--output") {
            if (i + 1 >= argc) {
                return false;
            }
            args.output = argv[++i];
        } else if (token == "--report-json") {
            if (i + 1 >= argc) {
                return false;
            }
            args.hasReportJson = true;
            args.reportJson = argv[++i];
        } else if (token == "--compiler") {
            if (i + 1 >= argc) {
                return false;
            }
            args.hasCompiler = true;
            args.compiler = argv[++i];
        } else {
            return false;
        }
    }

    return true;
}

void printUsage(const char *prog) {
    cerr << "Usage: " << prog
         << " <input-file> [--compiler <gcc|g++>] [--output <file>] [--report-json <file>]"
         << " [--no-compare] [--keep-binaries] [--fail-on-error]\\n";
}
