#pragma once
#include <cstdio>
#include <filesystem>
#include <string>

namespace ps2 {
inline std::filesystem::path host_path(const std::string& utf8) {
    return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
}
inline std::FILE* host_fopen(const std::string& utf8, const char* mode) {
#ifdef _WIN32
    std::wstring wide_mode;
    for (const char* p=mode; *p; ++p) wide_mode.push_back(*p);
    return _wfopen(host_path(utf8).c_str(), wide_mode.c_str());
#else
    return std::fopen(utf8.c_str(), mode);
#endif
}
}
