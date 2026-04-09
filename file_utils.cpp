#include "file_utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>

using namespace std;

bool fileExists(const string &path) {
    ifstream f(path.c_str());
    return f.good();
}

string getStem(const string &path) {
    const size_t slash = path.find_last_of("/\\");
    const string name = (slash == string::npos) ? path : path.substr(slash + 1);
    const size_t dot = name.find_last_of('.');
    return (dot == string::npos) ? name : name.substr(0, dot);
}

string getExtension(const string &path) {
    const size_t slash = path.find_last_of("/\\");
    const string name = (slash == string::npos) ? path : path.substr(slash + 1);
    const size_t dot = name.find_last_of('.');
    if (dot == string::npos) {
        return "";
    }

    string ext = name.substr(dot);
    transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(tolower(c));
    });
    return ext;
}
