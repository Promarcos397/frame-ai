#pragma once
#include <string>
#include <vector>

std::string trim(const std::string& s);
std::string base64_encode(const std::vector<unsigned char>& data);
std::string read_file(const std::string& path);
void write_file(const std::string& path, const std::string& content);
