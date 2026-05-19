#ifndef GOLDEN_MASTER_HPP
#define GOLDEN_MASTER_HPP

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

namespace golden {

inline bool updateModeEnabled() {
#if defined(GOLDEN_UPDATE_MODE)
    return true;
#else
    const char* env = std::getenv("GOLDEN_UPDATE");
    return env != nullptr && env[0] == '1' && env[1] == '\0';
#endif
}

inline std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

inline void writeFile(const std::filesystem::path& path, const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open()) << "Cannot write golden file: " << path;
    out << content;
}

inline std::string normalizeNewlines(std::string text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            out.push_back('\n');
        } else {
            out.push_back(text[i]);
        }
    }
    return out;
}

inline std::filesystem::path goldenPath(const std::string& suiteName,
                                        const std::string& testName) {
#if defined(GOLDEN_EXPECTED_DIR)
    return std::filesystem::path(GOLDEN_EXPECTED_DIR) / suiteName / (testName + ".golden.txt");
#else
    return std::filesystem::path("test/golden") / suiteName / (testName + ".golden.txt");
#endif
}

inline void assertMatchesGolden(const std::string& suiteName,
                                const std::string& testName,
                                const std::string& actualRaw) {
    const std::string actual = normalizeNewlines(actualRaw);
    const std::filesystem::path path = goldenPath(suiteName, testName);

    if (updateModeEnabled()) {
        writeFile(path, actual);
        GTEST_LOG_(INFO) << "Updated golden: " << path;
        return;
    }

    if (!std::filesystem::exists(path)) {
        const std::filesystem::path received =
            path.parent_path() / (testName + ".received.txt");
        writeFile(received, actual);
        FAIL() << "Missing golden file: " << path << "\n"
               << "Wrote actual output to: " << received << "\n"
               << "Re-run with -DUPDATE_GOLDEN_MASTER=ON or GOLDEN_UPDATE=1";
    }

    const std::string expected = normalizeNewlines(readFile(path));
    if (actual != expected) {
        const std::filesystem::path received =
            path.parent_path() / (testName + ".received.txt");
        writeFile(received, actual);
        EXPECT_EQ(expected, actual)
            << "Golden mismatch. Expected: " << path << "\n"
            << "Received: " << received;
    }
}

}  // namespace golden

#endif
