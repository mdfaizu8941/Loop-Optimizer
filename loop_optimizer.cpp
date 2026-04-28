#include "cli.h"
#include "file_utils.h"
#include "loop_optimizer.h"
#include "performance.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>

using namespace std;

string jsonEscape(const string &s) {
    string out;
    out.reserve(s.size());
    for (char ch : s) {
        switch (ch) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out.push_back(ch);
                break;
        }
    }
    return out;
}

bool writeJsonReport(
    const string &path,
    const OptimizationReport &report,
    const string &input,
    const string &output,
    const string &compiler
) {
    ofstream out(path);
    if (!out) {
        return false;
    }

    out << "{\n";
    out << "  \"input\": \"" << jsonEscape(input) << "\",\n";
    out << "  \"output\": \"" << jsonEscape(output) << "\",\n";
    out << "  \"compiler\": \"" << jsonEscape(compiler) << "\",\n";
    out << "  \"loopsDetected\": " << report.loopsDetected << ",\n";
    out << "  \"loopsUnrolled\": " << report.loopsUnrolled << ",\n";
    out << "  \"loopsSkippedSafety\": " << report.loopsSkippedSafety << ",\n";
    out << "  \"hoistedStatements\": " << report.hoistedStatements << ",\n";
    out << "  \"commonSubexpressionsEliminated\": " << report.commonSubexpressionsEliminated << ",\n";
    out << "  \"algebraicSimplifications\": " << report.algebraicSimplifications << ",\n";
    out << "  \"strengthReductions\": " << report.strengthReductions << ",\n";
    out << "  \"deadCodeRemoved\": " << report.deadCodeRemoved << ",\n";
    out << "  \"transformsFailed\": " << report.transformsFailed << ",\n";
    out << "  \"warningsCount\": " << report.warningsCount << ",\n";
    out << "  \"elapsedMs\": " << report.elapsedMs << "\n";
    out << "}\n";

    out.flush();
    return static_cast<bool>(out);
}

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
    string optimized;
    OptimizationReport report;

    try {
        const auto start = chrono::steady_clock::now();
        pair<string, OptimizationReport> optimizedRes = optimizeSource(source);
        const auto end = chrono::steady_clock::now();

        optimized = optimizedRes.first;
        report = optimizedRes.second;
        report.elapsedMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    } catch (const exception &ex) {
        ++report.transformsFailed;
        ++report.warningsCount;
        cerr << "Optimization failed: " << ex.what() << "\n";
        if (args.failOnError) {
            return 2;
        }
        optimized = source;
    } catch (...) {
        ++report.transformsFailed;
        ++report.warningsCount;
        cerr << "Optimization failed: unknown error\n";
        if (args.failOnError) {
            return 2;
        }
        optimized = source;
    }

    ofstream out(args.output);
    if (!out) {
        cerr << "Failed to write output: " << args.output << "\n";
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
    cout << "Loops skipped (safety): " << report.loopsSkippedSafety << "\n";
    cout << "LICM hoists: " << report.hoistedStatements << "\n";
    cout << "CSE eliminations: " << report.commonSubexpressionsEliminated << "\n";
    cout << "Algebraic simplifications: " << report.algebraicSimplifications << "\n";
    cout << "Strength reductions: " << report.strengthReductions << "\n";
    cout << "Dead code removed: " << report.deadCodeRemoved << "\n";
    cout << "Transform failures: " << report.transformsFailed << "\n";
    cout << "Warnings: " << report.warningsCount << "\n";
    cout << "Elapsed (ms): " << report.elapsedMs << "\n";
    cout << "Output: " << args.output << "\n";
    cout << "Compiler: " << compiler << "\n";

    if (args.hasReportJson) {
        if (!writeJsonReport(args.reportJson, report, args.input, args.output, compiler)) {
            cerr << "Failed to write JSON report: " << args.reportJson << "\n";
            if (args.failOnError) {
                return 2;
            }
        } else {
            cout << "JSON report: " << args.reportJson << "\n";
        }
    }

    if (!args.noCompare) {
        performanceCompare(args.input, args.output, compiler, args.keepBinaries);
    }

    if (args.failOnError && report.transformsFailed > 0) {
        return 2;
    }

    return 0;
}
