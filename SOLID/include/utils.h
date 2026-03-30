#pragma once
#include <string>
#include <ctime>

// SRP: Utility functions — stateless helpers only
// These are pure functions with no side effects or GTK dependencies

namespace Utils {

    // Time helpers
    std::string formatTimestamp(time_t t, const std::string& fmt = "%Y-%m-%d %H:%M");
    std::string currentTimestampForPath();   // e.g. "20260330_2245"

    // String helpers
    std::string  baseName(const std::string& path);
    std::string  joinPath(const std::string& dir, const std::string& file);
    bool         startsWith(const std::string& s, const std::string& prefix);

    // Number helpers
    std::string  formatSize(long bytes);
    std::string  intToStr(int value);

} // namespace Utils
