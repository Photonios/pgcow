#include <string>

#include <libzfs.h>

#include <pgcow/zfs/error.h>

namespace pgcow {
namespace zfs {

std::string error_description(libzfs_handle_t *zfs) {
    std::string error_description = std::to_string(libzfs_errno(zfs));
    error_description += " - ";
    error_description += libzfs_error_action(zfs);
    error_description += ": ";
    error_description += libzfs_error_description(zfs);

    return error_description;
}

} // namespace zfs
} // namespace pgcow
