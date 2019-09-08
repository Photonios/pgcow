#include <string>

#include <libzfs.h>

namespace pgcow {
namespace zfs {

/**
 * Gets a string describing the last error that occurred as a result
 * of calling libzfs functions.
 */
std::string error_description(libzfs_handle_t *zfs);

} // namespace zfs
} // namespace pgcow
