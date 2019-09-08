#include <filesystem>
#include <string>

#include <pgcow/fs.h>

namespace pgcow {
namespace fs {
namespace path {

std::string join(const std::string &path_a, const std::string &path_b) {
    std::filesystem::path joined_path(path_a);
    joined_path.append(path_b);

    return joined_path.u8string();
}

std::string leaf(const std::string &path) {
    std::filesystem::path p(path);
    return (*std::next(p.end(), -1)).u8string();
}

} // namespace path

} // namespace fs
} // namespace pgcow
