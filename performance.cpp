#include "performance.h"

#include "file_utils.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

using namespace std;

namespace {

bool compilerExists(const string &compiler) {
#ifdef _WIN32
    const string cmd = "where " + compiler + " >nul 2>&1";
#else
    const string cmd = "command -v " + compiler + " >/dev/null 2>&1";
#endif
    return system(cmd.c_str()) == 0;
}

bool compileSource(const string &compiler, const string &source, const string &exe) {
    const string cmd = compiler + " -O0 \"" + source + "\" -o \"" + exe + "\"";
    return system(cmd.c_str()) == 0;
}

double runAndTime(const string &exe) {
    auto start = chrono::high_resolution_clock::now();
#ifdef _WIN32
    const string runCmd = "\"" + exe + "\" >nul 2>&1";
#else
    const string runCmd = "\"./" + exe + "\" >/dev/null 2>&1";
#endif
    (void)system(runCmd.c_str());
    auto end = chrono::high_resolution_clock::now();
    return chrono::duration<double>(end - start).count();
}

}  // namespace

void performanceCompare(const string &inputFile, const string &outputFile, const string &compiler, bool keep) {
    if (!compilerExists(compiler)) {
        cout << "\nTiming skipped: '" << compiler << "' not found.\n";
        return;
    }

    const string base = getStem(inputFile);
    const string origExe = base + "_orig.exe";
    const string optExe = base + "_opt.exe";

    if (!compileSource(compiler, inputFile, origExe) || !compileSource(compiler, outputFile, optExe)) {
        cerr << "\nTiming error: failed to compile one or both binaries.\n";
        if (!keep) {
            remove(origExe.c_str());
            remove(optExe.c_str());
        }
        return;
    }

    const double original = runAndTime(origExe);
    const double optimized = runAndTime(optExe);
    const double improvement = original > 0.0 ? ((original - optimized) / original) * 100.0 : 0.0;

    cout << "\n=== Execution Time Comparison ===\n";
    cout << fixed << setprecision(6);
    cout << inputFile << ": " << original << "s\n";
    cout << outputFile << ": " << optimized << "s\n";
    cout << "Improvement (" << outputFile << " vs " << inputFile << "): " << improvement << "%\n";

    if (!keep) {
        remove(origExe.c_str());
        remove(optExe.c_str());
    }
}
