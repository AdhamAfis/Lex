#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>

inline std::string readFileContents(const std::string& filename) {
    if (std::filesystem::is_directory(filename))
        throw std::runtime_error("Could not read file: " + filename);
    std::ifstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("Could not open file: " + filename);
    std::string result;
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)) || file.gcount())
        result.append(buffer, static_cast<size_t>(file.gcount()));
    if (file.bad() || !file.eof()) throw std::runtime_error("Could not read file: " + filename);
    return result;
}

inline bool writeFileContents(const std::string& filename, const std::string& content) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path target(filename);
    if (fs::is_symlink(target, ec)) {
        target = fs::canonical(target, ec);
        if (ec) return false;
    }
    if (fs::exists(target, ec) && !fs::is_regular_file(target, ec)) return false;
    if (ec) return false;
    std::string pattern = (target.parent_path() / ("." + target.filename().string() + ".XXXXXX")).string();
    std::vector<char> temp(pattern.begin(), pattern.end());
    temp.push_back('\0');
    int fd = mkstemp(temp.data());
    if (fd < 0) return false;
    struct stat info;
    bool ok = true;
    if (stat(target.c_str(), &info) == 0) ok = fchmod(fd, info.st_mode & 07777) == 0;
    FILE* file = fdopen(fd, "wb");
    if (!file) { close(fd); std::remove(temp.data()); return false; }
    if (ok && std::fwrite(content.data(), 1, content.size(), file) != content.size()) ok = false;
    if (std::fclose(file) != 0) ok = false;
    if (ok) { fs::rename(temp.data(), target, ec); ok = !ec; }
    if (!ok) std::remove(temp.data());
    return ok;
}
#endif
