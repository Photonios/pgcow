#include <string>

namespace pgcow {
namespace fs {
namespace path {

/**
 * Joins the specified two paths together (often path A is absolute).
 *
 * This takes care of the path separate magic. It makes sure A and B
 * are separated by a path separator and won't add one when A already
 * ends in one or B starts with one.
 */
std::string join(const std::string &path_a, const std::string &path_b);

/**
 * Gets the last segment of the specified path.
 *
 * path="/home/swen/bla" gives "bla"
 * path="/home/swen/test.txt" gives "test.txt".
 */
std::string leaf(const std::string &path);

} // namespace path

} // namespace fs
} // namespace pgcow
