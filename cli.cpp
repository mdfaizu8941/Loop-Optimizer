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
    cerr << "Usage: " << prog << " <input-file> [--compiler <gcc|g++>] [--no-compare] [--keep-binaries]\\n";
}
