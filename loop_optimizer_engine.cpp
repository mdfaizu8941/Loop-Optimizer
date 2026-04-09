#include "loop_optimizer.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

namespace {

struct LoopSpan {
    string kind;
    size_t start;
    string header;
    size_t bodyStart;
    size_t bodyEnd;
};

struct AssignmentInfo {
    string lhs;
    string rhs;
    string typePrefix;
    bool isDeclaration;
};

struct ForInfo {
    string var;
    string init;
    string op;
    string bound;
    string cond;
    bool initDecl;
};

const regex RE_IDENTIFIER(R"(\b[A-Za-z_]\w*\b)");
const regex RE_TYPE_PREFIX(
    R"(^(?:unsigned|signed|short|long|int|char|float|double|size_t|u?int\d*_t|const|volatile|static|register|(?:struct|enum)\s+\w+))"
);
const regex RE_POWER_OF_TWO_MUL(R"(\b([A-Za-z_]\w*)\s*\*\s*(2|4|8|16|32|64)\b)");
const regex RE_POWER_OF_TWO_MUL_REV(R"(\b(2|4|8|16|32|64)\s*\*\s*([A-Za-z_]\w*)\b)");
const regex RE_DECL_ASSIGN(R"(^\s*([A-Za-z_][\w\s*]+?)\s+([A-Za-z_]\w*)\s*=\s*(.+);\s*$)");
const regex RE_PLAIN_ASSIGN(R"(^\s*([A-Za-z_]\w*)\s*=\s*(.+);\s*$)");
const regex RE_MODIFY_OP(R"(\b([A-Za-z_]\w*)\s*(\+\+|--|\+=|-=|\*=|/=|%=))");
const regex RE_CONTROL_FLOW(R"(\b(break|continue|goto|return)\b)");

const unordered_set<string> C_KEYWORDS = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline",
    "int", "long", "register", "restrict", "return", "short", "signed", "sizeof",
    "static", "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile", "while", "_Bool", "_Complex", "_Imaginary",
    "alignas", "alignof", "bool", "catch", "class", "constexpr", "consteval",
    "constinit", "decltype", "delete", "explicit", "export", "false", "friend",
    "mutable", "namespace", "new", "noexcept", "nullptr", "operator", "private",
    "protected", "public", "reinterpret_cast", "static_assert", "template", "this",
    "throw", "true", "try", "typename", "using", "virtual"
};

string trim(const string &s) {
    size_t begin = 0;
    while (begin < s.size() && isspace(static_cast<unsigned char>(s[begin]))) {
        ++begin;
    }
    size_t end = s.size();
    while (end > begin && isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(begin, end - begin);
}

bool isWordChar(char c) {
    return isalnum(static_cast<unsigned char>(c)) || c == '_';
}

bool isWordBoundary(const string &text, size_t idx, const string &token) {
    const bool before = idx == 0 || !isWordChar(text[idx - 1]);
    const size_t afterIdx = idx + token.size();
    const bool after = afterIdx >= text.size() || !isWordChar(text[afterIdx]);
    return before && after;
}

size_t skipWhitespace(const string &text, size_t idx) {
    while (idx < text.size() && isspace(static_cast<unsigned char>(text[idx]))) {
        ++idx;
    }
    return idx;
}

int findMatching(const string &text, size_t start, char openCh) {
    const char closeCh = (openCh == '(') ? ')' : '}';
    int depth = 0;
    bool inString = false;
    bool escapeNext = false;
    char stringChar = '\0';

    for (size_t i = start; i < text.size(); ++i) {
        const char ch = text[i];
        if (escapeNext) {
            escapeNext = false;
        } else if (inString) {
            if (ch == '\\') {
                escapeNext = true;
            } else if (ch == stringChar) {
                inString = false;
            }
        } else if (ch == '"' || ch == '\'') {
            inString = true;
            stringChar = ch;
        } else if (ch == openCh) {
            ++depth;
        } else if (ch == closeCh) {
            --depth;
            if (depth == 0) {
                return static_cast<int>(i);
            }
        }
    }
    return -1;
}

vector<string> splitStatements(const string &block) {
    vector<string> statements;
    string current;
    int paren = 0;
    int brace = 0;
    bool inString = false;
    bool escapeNext = false;
    char quote = '\0';

    for (char ch : block) {
        current.push_back(ch);
        if (escapeNext) {
            escapeNext = false;
            continue;
        }
        if (inString) {
            if (ch == '\\') {
                escapeNext = true;
            } else if (ch == quote) {
                inString = false;
            }
            continue;
        }
        if (ch == '"' || ch == '\'') {
            inString = true;
            quote = ch;
        } else if (ch == '(') {
            ++paren;
        } else if (ch == ')') {
            paren = max(0, paren - 1);
        } else if (ch == '{') {
            ++brace;
        } else if (ch == '}') {
            brace = max(0, brace - 1);
        } else if (ch == ';' && paren == 0 && brace == 0) {
            string s = trim(current);
            if (!s.empty()) {
                statements.push_back(s);
            }
            current.clear();
        }
    }

    string tail = trim(current);
    if (!tail.empty()) {
        statements.push_back(tail);
    }
    return statements;
}

string stripComments(const string &text) {
    string out;
    out.reserve(text.size());
    bool inString = false;
    bool escapeNext = false;
    char quote = '\0';

    for (size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        const char next = (i + 1 < text.size()) ? text[i + 1] : '\0';

        if (escapeNext) {
            out.push_back(ch);
            escapeNext = false;
            continue;
        }

        if (inString) {
            out.push_back(ch);
            if (ch == '\\') {
                escapeNext = true;
            } else if (ch == quote) {
                inString = false;
            }
            continue;
        }

        if (ch == '"' || ch == '\'') {
            inString = true;
            quote = ch;
            out.push_back(ch);
            continue;
        }

        if (ch == '/' && next == '/') {
            i += 2;
            while (i < text.size() && text[i] != '\n' && text[i] != '\r') {
                ++i;
            }
            if (i < text.size()) {
                out.push_back(text[i]);
            }
            continue;
        }

        if (ch == '/' && next == '*') {
            i += 2;
            while (i < text.size()) {
                if (i + 1 < text.size() && text[i] == '*' && text[i + 1] == '/') {
                    ++i;
                    break;
                }
                ++i;
            }
            continue;
        }

        out.push_back(ch);
    }

    return trim(out);
}

unordered_set<string> extractIdentifiers(const string &expr) {
    unordered_set<string> out;
    for (sregex_iterator it(expr.begin(), expr.end(), RE_IDENTIFIER), end; it != end; ++it) {
        string ident = (*it).str();
        if (C_KEYWORDS.find(ident) == C_KEYWORDS.end()) {
            out.insert(ident);
        }
    }
    return out;
}

vector<LoopSpan> detectLoops(const string &code) {
    vector<LoopSpan> loops;
    size_t i = 0;
    const size_t n = code.size();

    while (i < n) {
        bool matchedLoop = false;
        for (const string keyword : {string("for"), string("while")}) {
            if (i + keyword.size() <= n && code.compare(i, keyword.size(), keyword) == 0 && isWordBoundary(code, i, keyword)) {
                const size_t parenStart = skipWhitespace(code, i + keyword.size());
                if (parenStart >= n || code[parenStart] != '(') {
                    continue;
                }

                const int parenEndI = findMatching(code, parenStart, '(');
                if (parenEndI < 0) {
                    continue;
                }
                const size_t parenEnd = static_cast<size_t>(parenEndI);

                const string header = code.substr(parenStart + 1, parenEnd - parenStart - 1);
                const size_t bodyStart = skipWhitespace(code, parenEnd + 1);
                if (bodyStart >= n) {
                    continue;
                }

                size_t bodyEnd = string::npos;
                if (code[bodyStart] == '{') {
                    const int bodyEndI = findMatching(code, bodyStart, '{');
                    if (bodyEndI >= 0) {
                        bodyEnd = static_cast<size_t>(bodyEndI);
                    }
                } else {
                    bodyEnd = code.find(';', bodyStart);
                }
                if (bodyEnd == string::npos) {
                    continue;
                }

                loops.push_back(LoopSpan{keyword, i, header, bodyStart, bodyEnd});
                i = bodyEnd + 1;
                matchedLoop = true;
                break;
            }
        }
        if (!matchedLoop) {
            ++i;
        }
    }

    return loops;
}

string replacePowerMulPattern(
    const string &stmt,
    const regex &pattern,
    int varGroup,
    int constGroup,
    int &count
) {
    string out;
    size_t last = 0;

    for (sregex_iterator it(stmt.begin(), stmt.end(), pattern), end; it != end; ++it) {
        const smatch m = *it;
        out += stmt.substr(last, static_cast<size_t>(m.position()) - last);

        const int constant = stoi(m.str(constGroup));
        if (constant > 0 && (constant & (constant - 1)) == 0) {
            ++count;
            int shift = 0;
            int v = constant;
            while (v > 1) {
                v >>= 1;
                ++shift;
            }
            out += m.str(varGroup) + " << " + to_string(shift);
        } else {
            out += m.str(0);
        }

        last = static_cast<size_t>(m.position() + m.length());
    }

    out += stmt.substr(last);
    return out;
}

pair<string, int> applyStrengthReduction(const string &stmt) {
    int count = 0;
    string out = replacePowerMulPattern(stmt, RE_POWER_OF_TWO_MUL, 1, 2, count);
    out = replacePowerMulPattern(out, RE_POWER_OF_TWO_MUL_REV, 2, 1, count);
    return {out, count};
}

bool parseAssignment(const string &stmt, AssignmentInfo &outInfo);

string stripWrappingParens(string expr) {
    expr = trim(expr);
    while (expr.size() >= 2 && expr.front() == '(' && expr.back() == ')') {
        const int endIdx = findMatching(expr, 0, '(');
        if (endIdx != static_cast<int>(expr.size()) - 1) {
            break;
        }
        expr = trim(expr.substr(1, expr.size() - 2));
    }
    return expr;
}

string normalizeExpression(const string &expr) {
    string compact;
    compact.reserve(expr.size());
    for (char ch : stripWrappingParens(expr)) {
        if (!isspace(static_cast<unsigned char>(ch))) {
            compact.push_back(ch);
        }
    }
    return compact;
}

bool isPureArithmeticExpression(const string &expr) {
    const string t = trim(expr);
    if (t.empty()) {
        return false;
    }
    if (t.find("++") != string::npos || t.find("--") != string::npos) {
        return false;
    }
    if (t.find('=') != string::npos || t.find(',') != string::npos || t.find('?') != string::npos || t.find(':') != string::npos) {
        return false;
    }

    const regex reFunctionCall(R"(\b[A-Za-z_]\w*\s*\()");
    if (regex_search(t, reFunctionCall)) {
        return false;
    }

    const string allowedOps = "+-*/%().<>!&|^~";
    for (char ch : t) {
        const bool ok = isalnum(static_cast<unsigned char>(ch)) ||
                        ch == '_' ||
                        isspace(static_cast<unsigned char>(ch)) ||
                        allowedOps.find(ch) != string::npos;
        if (!ok) {
            return false;
        }
    }
    return true;
}

size_t findTopLevelBinaryOp(const string &expr, char op) {
    int paren = 0;
    bool inString = false;
    bool escapeNext = false;
    char quote = '\0';

    for (size_t i = 0; i < expr.size(); ++i) {
        const char ch = expr[i];
        if (escapeNext) {
            escapeNext = false;
            continue;
        }
        if (inString) {
            if (ch == '\\') {
                escapeNext = true;
            } else if (ch == quote) {
                inString = false;
            }
            continue;
        }
        if (ch == '"' || ch == '\'') {
            inString = true;
            quote = ch;
            continue;
        }
        if (ch == '(') {
            ++paren;
            continue;
        }
        if (ch == ')') {
            paren = max(0, paren - 1);
            continue;
        }
        if (paren != 0 || ch != op) {
            continue;
        }
        if (op == '+' && i + 1 < expr.size() && expr[i + 1] == '+') {
            continue;
        }
        if (op == '+' && i > 0 && expr[i - 1] == '+') {
            continue;
        }
        return i;
    }
    return string::npos;
}

pair<string, bool> simplifyAlgebraicExpression(const string &rhs) {
    const string expr = trim(rhs);

    const size_t plusPos = findTopLevelBinaryOp(expr, '+');
    if (plusPos != string::npos) {
        const string left = trim(expr.substr(0, plusPos));
        const string right = trim(expr.substr(plusPos + 1));
        if (!left.empty() && !right.empty() && normalizeExpression(left) == normalizeExpression(right)) {
            const string operand = stripWrappingParens(left);
            if (!operand.empty()) {
                if (regex_match(operand, RE_IDENTIFIER)) {
                    return {"2 * " + operand, true};
                }
                return {"2 * (" + operand + ")", true};
            }
        }
    }

    const size_t mulPos = findTopLevelBinaryOp(expr, '*');
    if (mulPos != string::npos) {
        const string left = trim(expr.substr(0, mulPos));
        const string right = trim(expr.substr(mulPos + 1));
        if (normalizeExpression(left) == "0" || normalizeExpression(right) == "0") {
            return {"0", true};
        }
    }

    return {expr, false};
}

bool isLoopInvariantExpression(
    const string &rhs,
    const unordered_set<string> &loopVars,
    const unordered_set<string> &modifiedVars
) {
    if (!isPureArithmeticExpression(rhs)) {
        return false;
    }

    const auto ids = extractIdentifiers(rhs);
    for (const string &id : ids) {
        if (loopVars.find(id) != loopVars.end()) {
            return false;
        }
        if (modifiedVars.find(id) != modifiedVars.end()) {
            return false;
        }
    }
    return true;
}

unordered_map<string, int> detectDuplicateExpressions(const vector<string> &statements) {
    unordered_map<string, int> counts;
    for (const string &stmt : statements) {
        AssignmentInfo assign;
        if (!parseAssignment(stmt, assign)) {
            continue;
        }
        if (!isPureArithmeticExpression(assign.rhs)) {
            continue;
        }
        ++counts[normalizeExpression(assign.rhs)];
    }
    return counts;
}

string rebuildAssignment(const AssignmentInfo &assign, const string &rhs) {
    if (assign.isDeclaration) {
        return assign.typePrefix + " " + assign.lhs + " = " + rhs + ";";
    }
    return assign.lhs + " = " + rhs + ";";
}

bool parseAssignment(const string &stmt, AssignmentInfo &outInfo) {
    smatch m;
    if (regex_match(stmt, m, RE_DECL_ASSIGN)) {
        const string typePrefix = trim(m.str(1));
        if (regex_search(typePrefix, RE_TYPE_PREFIX)) {
            outInfo = AssignmentInfo{m.str(2), trim(m.str(3)), typePrefix, true};
            return true;
        }
    }
    if (regex_match(stmt, m, RE_PLAIN_ASSIGN)) {
        outInfo = AssignmentInfo{m.str(1), trim(m.str(2)), "", false};
        return true;
    }
    return false;
}

string getBodyInner(const string &body) {
    const string s = trim(body);
    if (s.size() >= 2 && s.front() == '{' && s.back() == '}') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

pair<vector<string>, string> optimizeLoopBody(
    const string &body,
    const string *loopVar,
    OptimizationReport &report
) {
    const vector<string> rawStatements = splitStatements(getBodyInner(body));
    vector<string> statements;
    statements.reserve(rawStatements.size());
    for (const string &raw : rawStatements) {
        const string cleaned = stripComments(raw);
        if (!cleaned.empty()) {
            statements.push_back(cleaned);
        }
    }

    vector<string> simplified;
    simplified.reserve(statements.size());
    for (const string &stmt : statements) {
        pair<string, int> reducedRes = applyStrengthReduction(stmt);
        string reducedStmt = reducedRes.first;
        report.strengthReductions += reducedRes.second;

        AssignmentInfo assign;
        if (parseAssignment(reducedStmt, assign)) {
            const auto simp = simplifyAlgebraicExpression(assign.rhs);
            if (simp.second) {
                ++report.algebraicSimplifications;
                reducedStmt = rebuildAssignment(assign, simp.first);
            }
        }
        simplified.push_back(trim(reducedStmt));
    }

    const unordered_map<string, int> duplicateCounts = detectDuplicateExpressions(simplified);

    vector<string> cseApplied;
    cseApplied.reserve(simplified.size());
    unordered_map<string, string> exprRepresentative;
    unordered_map<string, unordered_set<string>> exprDependencies;

    for (const string &stmt : simplified) {
        AssignmentInfo assign;
        bool hasAssign = parseAssignment(stmt, assign);
        string current = stmt;
        string producedKey;

        if (hasAssign && isPureArithmeticExpression(assign.rhs)) {
            const string key = normalizeExpression(assign.rhs);
            const auto cntIt = duplicateCounts.find(key);
            const bool duplicate = cntIt != duplicateCounts.end() && cntIt->second > 1;
            if (duplicate) {
                auto repIt = exprRepresentative.find(key);
                if (repIt != exprRepresentative.end() && repIt->second != assign.lhs) {
                    current = rebuildAssignment(assign, repIt->second);
                    ++report.commonSubexpressionsEliminated;
                } else {
                    exprRepresentative[key] = assign.lhs;
                    exprDependencies[key] = extractIdentifiers(assign.rhs);
                    producedKey = key;
                }
            }
        }

        cseApplied.push_back(current);

        unordered_set<string> modifiedInStmt;
        AssignmentInfo currentAssign;
        if (parseAssignment(current, currentAssign)) {
            modifiedInStmt.insert(currentAssign.lhs);
        }
        for (sregex_iterator it(current.begin(), current.end(), RE_MODIFY_OP), end; it != end; ++it) {
            modifiedInStmt.insert((*it).str(1));
        }

        for (const string &modified : modifiedInStmt) {
            for (auto it = exprRepresentative.begin(); it != exprRepresentative.end(); ) {
                const string &key = it->first;
                const string &rep = it->second;
                const bool keepCurrentProduced = !producedKey.empty() && key == producedKey;
                const auto depIt = exprDependencies.find(key);
                const bool dependsOnModified = depIt != exprDependencies.end() && depIt->second.find(modified) != depIt->second.end();
                if (!keepCurrentProduced && (rep == modified || dependsOnModified)) {
                    exprDependencies.erase(key);
                    it = exprRepresentative.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    unordered_set<string> modifiedVars;
    for (const string &stmt : cseApplied) {
        AssignmentInfo assign;
        if (parseAssignment(stmt, assign)) {
            modifiedVars.insert(assign.lhs);
        }
        for (sregex_iterator it(stmt.begin(), stmt.end(), RE_MODIFY_OP), end; it != end; ++it) {
            modifiedVars.insert((*it).str(1));
        }
    }

    unordered_set<string> loopVars;
    if (loopVar != nullptr) {
        loopVars.insert(*loopVar);
    }

    vector<string> hoisted;
    vector<string> remain;
    for (const string &stmt : cseApplied) {
        AssignmentInfo assign;
        if (parseAssignment(stmt, assign) && isLoopInvariantExpression(assign.rhs, loopVars, modifiedVars)) {
            hoisted.push_back(stmt);
            ++report.hoistedStatements;
            continue;
        }
        remain.push_back(stmt);
    }

    // Reuse already-hoisted invariant expressions inside the loop body.
    unordered_map<string, string> hoistedExprRepresentative;
    for (const string &stmt : hoisted) {
        AssignmentInfo assign;
        if (!parseAssignment(stmt, assign)) {
            continue;
        }
        if (!isPureArithmeticExpression(assign.rhs)) {
            continue;
        }
        const string key = normalizeExpression(assign.rhs);
        if (!key.empty() && hoistedExprRepresentative.find(key) == hoistedExprRepresentative.end()) {
            hoistedExprRepresentative[key] = assign.lhs;
        }
    }

    vector<string> reusedRemain;
    reusedRemain.reserve(remain.size());
    for (const string &stmt : remain) {
        AssignmentInfo assign;
        if (parseAssignment(stmt, assign) && isPureArithmeticExpression(assign.rhs)) {
            const string key = normalizeExpression(assign.rhs);
            const auto repIt = hoistedExprRepresentative.find(key);
            if (repIt != hoistedExprRepresentative.end() && repIt->second != assign.lhs) {
                reusedRemain.push_back(rebuildAssignment(assign, repIt->second));
                ++report.commonSubexpressionsEliminated;
                continue;
            }
        }
        reusedRemain.push_back(stmt);
    }

    remain = move(reusedRemain);

    unordered_set<string> declared;
    for (const string &s : remain) {
        AssignmentInfo a;
        if (parseAssignment(s, a) && a.isDeclaration) {
            declared.insert(a.lhs);
        }
    }

    unordered_set<string> live;
    vector<string> final;
    for (auto it = remain.rbegin(); it != remain.rend(); ++it) {
        const string &stmt = *it;
        AssignmentInfo assign;
        if (parseAssignment(stmt, assign)) {
            const bool declaredInsideLoop = declared.find(assign.lhs) != declared.end();
            const bool removableDeadStore = declaredInsideLoop &&
                                            live.find(assign.lhs) == live.end() &&
                                            isPureArithmeticExpression(assign.rhs);
            if (removableDeadStore) {
                ++report.deadCodeRemoved;
                continue;
            }

            live.erase(assign.lhs);
            const auto ids = extractIdentifiers(assign.rhs);
            live.insert(ids.begin(), ids.end());
        } else {
            const auto ids = extractIdentifiers(stmt);
            live.insert(ids.begin(), ids.end());
        }
        final.push_back(stmt);
    }

    reverse(final.begin(), final.end());

    ostringstream inner;
    for (size_t i = 0; i < final.size(); ++i) {
        inner << "    " << trim(final[i]);
        if (i + 1 < final.size()) {
            inner << '\n';
        }
    }

    const string innerText = inner.str();
    const string bodyText = innerText.empty() ? "{}" : "{\n" + innerText + "\n}";
    return {hoisted, bodyText};
}

string optimizeLoop(const LoopSpan &loop, const string &body, OptimizationReport &report);

string optimizeNestedLoopsInBody(const string &body, OptimizationReport &report) {
    string inner = getBodyInner(body);
    const vector<LoopSpan> innerLoops = detectLoops(inner);
    if (innerLoops.empty()) {
        return body;
    }

    report.loopsDetected += static_cast<int>(innerLoops.size());
    for (auto it = innerLoops.rbegin(); it != innerLoops.rend(); ++it) {
        const LoopSpan &innerLoop = *it;
        const string innerBody = inner.substr(innerLoop.bodyStart, innerLoop.bodyEnd - innerLoop.bodyStart + 1);
        const string optimized = optimizeLoop(innerLoop, innerBody, report);
        inner = inner.substr(0, innerLoop.start) + optimized + inner.substr(innerLoop.bodyEnd + 1);
    }

    return "{\n" + inner + "\n}";
}

bool parseSimpleFor(const string &header, ForInfo &outInfo) {
    vector<string> parts;
    string cur;
    stringstream ss(header);
    while (getline(ss, cur, ';')) {
        parts.push_back(trim(cur));
    }
    if (parts.size() != 3) {
        return false;
    }

    const string init = parts[0];
    const string cond = parts[1];
    const string inc = parts[2];

    smatch m;
    const regex reInit(R"(^(?:[A-Za-z_][\w\s*]*\s+)?([A-Za-z_]\w*)\s*=\s*(-?\d+)\s*$)");
    if (!regex_match(init, m, reInit)) {
        return false;
    }
    const string var = m.str(1);
    const bool initDecl = regex_search(init, RE_TYPE_PREFIX);

    const regex reCond("^" + var + R"(\s*(<|<=)\s*(.+)$)");
    if (!regex_match(cond, m, reCond)) {
        return false;
    }
    const string op = m.str(1);
    const string bound = trim(m.str(2));

    const string incPattern =
        "^(?:" + var + R"(\+\+|\+\+)" + var +
        "|" + var + R"(\s*\+=\s*1|\s*=\s*)" + var + R"(\s*\+\s*1)$)";
    const regex reInc(incPattern);
    if (!regex_match(inc, reInc)) {
        return false;
    }

    outInfo = ForInfo{var, init, op, bound, cond, initDecl};
    return true;
}

string optimizeLoop(const LoopSpan &loop, const string &body, OptimizationReport &report) {
    const string nestedOptimizedBody = optimizeNestedLoopsInBody(body, report);

    bool hasLoopVar = false;
    string loopVar;
    bool parsed = false;
    ForInfo forInfo;

    if (loop.kind == "for") {
        parsed = parseSimpleFor(loop.header, forInfo);
        if (parsed) {
            loopVar = forInfo.var;
            hasLoopVar = true;
        }
    }

    auto bodyOpt = optimizeLoopBody(nestedOptimizedBody, hasLoopVar ? &loopVar : nullptr, report);
    vector<string> hoisted = bodyOpt.first;
    string optBody = bodyOpt.second;

    ostringstream hoistedText;
    for (size_t i = 0; i < hoisted.size(); ++i) {
        hoistedText << trim(hoisted[i]);
        if (i + 1 < hoisted.size()) {
            hoistedText << '\n';
        }
    }

    if (parsed && !regex_search(optBody, RE_CONTROL_FLOW)) {
        const string inner = trim(getBodyInner(optBody));
        bool hasDecl = false;
        for (const string &s : splitStatements(inner)) {
            AssignmentInfo a;
            if (parseAssignment(s, a) && a.isDeclaration) {
                hasDecl = true;
                break;
            }
        }

        if (!hasDecl) {
            ostringstream bodyLines;
            if (!inner.empty()) {
                stringstream ls(inner);
                string ln;
                bool first = true;
                while (getline(ls, ln)) {
                    if (!first) {
                        bodyLines << '\n';
                    }
                    bodyLines << "        " << ln;
                    first = false;
                }
            }

            ostringstream unrolled;
            if (forInfo.initDecl) {
                unrolled << "{\n    " << forInfo.init << ";\n"
                         << "for (; " << forInfo.var << " + 1 " << forInfo.op
                         << " " << forInfo.bound << "; " << forInfo.var << " += 2) {\n";
            } else {
                unrolled << "for (" << forInfo.init << "; " << forInfo.var << " + 1 " << forInfo.op
                         << " " << forInfo.bound << "; " << forInfo.var << " += 2) {\n";
            }
            if (!inner.empty()) {
                unrolled << bodyLines.str() << "\n";
            }
            unrolled << "        " << forInfo.var << "++;\n";
            if (!inner.empty()) {
                unrolled << bodyLines.str() << "\n";
            }
            unrolled << "}\nfor (; " << forInfo.cond << "; " << forInfo.var << "++) " << optBody;
            if (forInfo.initDecl) {
                unrolled << "\n}";
            }

            ++report.loopsUnrolled;
            if (!hoisted.empty()) {
                return hoistedText.str() + "\n" + unrolled.str();
            }
            return unrolled.str();
        }
    }

    const string result = loop.kind + " (" + loop.header + ") " + optBody;
    if (!hoisted.empty()) {
        return hoistedText.str() + "\n" + result;
    }
    return result;
}

}  // namespace

pair<string, OptimizationReport> optimizeSource(string code) {
    OptimizationReport report;
    const vector<LoopSpan> loops = detectLoops(code);
    report.loopsDetected = static_cast<int>(loops.size());

    for (auto it = loops.rbegin(); it != loops.rend(); ++it) {
        const LoopSpan &loop = *it;
        const string body = code.substr(loop.bodyStart, loop.bodyEnd - loop.bodyStart + 1);
        const string optimized = optimizeLoop(loop, body, report);
        code = code.substr(0, loop.start) + optimized + code.substr(loop.bodyEnd + 1);
    }

    return {code, report};
}
