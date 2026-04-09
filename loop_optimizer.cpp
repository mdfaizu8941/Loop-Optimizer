#include "cli.h"
#include "file_utils.h"
#include "loop_optimizer.h"
#include "performance.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>

using namespace std;

static const string FIXED_OUTPUT_FILE = "ouput.c";

int main(int argc, char **argv) {
    Args args;
    if (!parseArgs(argc, argv, args)) {
        printUsage(argv[0]);
        return 1;
    }

    if (!fileExists(args.input)) {
        cerr << "File not found: " << args.input << "\n";
        return 1;
    }

    ifstream in(args.input);
    if (!in) {
        cerr << "Failed to open input: " << args.input << "\n";
        return 1;
    }

    string source((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    pair<string, OptimizationReport> optimizedRes = optimizeSource(source);
    string optimized = optimizedRes.first;
    OptimizationReport report = optimizedRes.second;

    ofstream out(FIXED_OUTPUT_FILE);
    if (!out) {
        cerr << "Failed to write output: " << FIXED_OUTPUT_FILE << "\n";
        return 1;
    }
    out << optimized;
    out.flush();
    out.close();

    string compiler;
    if (args.hasCompiler) {
        compiler = args.compiler;
    } else {
        const string ext = getExtension(args.input);
        compiler = (ext == ".cpp" || ext == ".cc" || ext == ".cxx") ? "g++" : "gcc";
    }

    cout << "=== Loop Optimization Report ===\n";
    cout << "Loops detected: " << report.loopsDetected << "\n";
    cout << "Loops unrolled: " << report.loopsUnrolled << "\n";
    cout << "LICM hoists: " << report.hoistedStatements << "\n";
    cout << "CSE eliminations: " << report.commonSubexpressionsEliminated << "\n";
    cout << "Algebraic simplifications: " << report.algebraicSimplifications << "\n";
    cout << "Strength reductions: " << report.strengthReductions << "\n";
    cout << "Dead code removed: " << report.deadCodeRemoved << "\n";
    cout << "Output: " << FIXED_OUTPUT_FILE << "\n";
    cout << "Compiler: " << compiler << "\n";

    if (!args.noCompare) {
        performanceCompare(args.input, FIXED_OUTPUT_FILE, compiler, args.keepBinaries);
    }

    return 0;
}
