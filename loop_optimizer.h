#pragma once

#include <string>
#include <utility>

struct OptimizationReport {
    int loopsDetected = 0;
    int loopsUnrolled = 0;
    int loopsSkippedSafety = 0;
    int hoistedStatements = 0;
    int commonSubexpressionsEliminated = 0;
    int algebraicSimplifications = 0;
    int strengthReductions = 0;
    int deadCodeRemoved = 0;
    int transformsFailed = 0;
    int warningsCount = 0;
    long long elapsedMs = 0;
};

std::pair<std::string, OptimizationReport> optimizeSource(std::string code);
