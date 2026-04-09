#include <iostream>
#include <regex>
using namespace std;

int main() {
    string text = "I have 25 apples";
    regex pattern("\\d+");

    if (regex_search(text, pattern)) {
        cout << "Number found";
    }
}