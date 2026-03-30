#include "utils.h"
#include <sstream>
#include <iomanip>
#include <cstring>

// SRP: stateless helpers only — no GTK, no business logic

namespace Utils {

std::string formatTimestamp(time_t t, const std::string& fmt) {
    struct tm* tmInfo = localtime(&t);
    char buf[128];
    strftime(buf, sizeof(buf), fmt.c_str(), tmInfo);
    return std::string(buf);
}

std::string currentTimestampForPath() {
    time_t now = time(nullptr);
    return formatTimestamp(now, "%Y%m%d_%H%M");
}

std::string baseName(const std::string& path) {
    // Works for both '/' and '\'
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

std::string joinPath(const std::string& dir, const std::string& file) {
    if (dir.empty()) return file;
    char last = dir.back();
    if (last == '/' || last == '\\')
        return dir + file;
#ifdef _WIN32
    return dir + "\\" + file;
#else
    return dir + "/" + file;
#endif
}

bool startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() &&
           s.compare(0, prefix.size(), prefix) == 0;
}

std::string formatSize(long bytes) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    if (bytes < 1024)
        oss << bytes << " B";
    else if (bytes < 1024L * 1024)
        oss << (bytes / 1024.0) << " KB";
    else if (bytes < 1024L * 1024 * 1024)
        oss << (bytes / (1024.0 * 1024.0)) << " MB";
    else
        oss << (bytes / (1024.0 * 1024.0 * 1024.0)) << " GB";
    return oss.str();
}

std::string intToStr(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

} // namespace Utils
