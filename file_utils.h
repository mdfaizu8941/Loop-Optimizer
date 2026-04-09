#pragma once

#include <string>

bool fileExists(const std::string &path);
std::string getStem(const std::string &path);
std::string getExtension(const std::string &path);
