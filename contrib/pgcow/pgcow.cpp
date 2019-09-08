#include <chrono>
#include <filesystem>

#include <pgcow/fs.h>
#include <pgcow/zfs/dataset.h>
#include <pgcow/postgres/extension.h>

extern "C" {

PG_MODULE_MAGIC;

/**
 * Next hooked copydir to invoke.
 *
 * If another extension also hooks into copydir we need to call
 * the next extension's hook instead of directly calling the standard
 * PostgreSQL implementation.
 */
static copydir_hook_type next_copydir_hook = NULL;

/**
 * Gets the current time as the number of milliseconds that
 * elapsed since January 1st, 1970 (UTC).
 */
static uint64_t current_time_ms() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

/**
 * Hooked copydir function.
 *
 * copydir is an internal PostgreSQL function that gets called
 * to copy a directory.
 *
 * CowDB installs a hook to use ZFS dataset snapshotting/cloning
 * behaviour instead of blindly copying the directory.
 */
static void intercept_copydir(char *fromdir, char *todir, bool recurse) {
    libzfs_handle_t *zfs = libzfs_init();
    if (!zfs) {
        ereport(ERROR,
                (errcode_for_file_access(), errmsg("cannot init libzfs")));
        return;
    }

    // paths passed to copydir() are relative to the current dir, make absolute
    std::string cwd = std::filesystem::current_path().u8string();
    std::string from_path = pgcow::fs::path::join(cwd, std::string(fromdir));

    // if `fromdir` is not a zfs dataset, do nothing and proceed normally
    auto dataset = pgcow::zfs::dataset::by_mountpoint(zfs, from_path);
    if (!dataset) {
        ereport(DEBUG4,
                (errmsg_internal("\"%s\" is not a zfs dataset", fromdir)));
        standard_copydir(fromdir, todir, recurse);
        return;
    }

    // create a snapshot of the dataset
    ereport(DEBUG4, (errmsg_internal("found zfs dataset \"%s\" for \"%s\"",
                                     dataset->name().c_str(), fromdir)));
    auto snapshot = dataset->snapshot(std::to_string(current_time_ms()));
    if (!snapshot) {
        ereport(ERROR,
                (errcode_for_file_access(),
                 errmsg("cannot create zfs snapshot of \"%s\"", fromdir)));
        return;
    }

    ereport(DEBUG4, (errmsg_internal("created zfs snapshot \"%s\"",
                                     snapshot->name().c_str())));

    // clone the snapshot into the target dir
    std::string clone_name = pgcow::fs::path::leaf(todir);
    auto clone = snapshot->clone(clone_name);
    if (!clone) {
        ereport(ERROR, (errcode_for_file_access(),
                        errmsg("cannot create zfs clone of \"%s\"",
                               snapshot->name().c_str())));
        return;
    }

    ereport(DEBUG4, (errmsg_internal("created zfs clone \"%s\"",
                                     clone->name().c_str())));

    // if some other plugin hooked copydir(), call that one
    if (next_copydir_hook) {
        ereport(DEBUG4, (errmsg_internal("pgcow handing off to next hook")));

        (*next_copydir_hook)(fromdir, todir, recurse);
        return;
    }
}

void _PG_init(void);

/**
 * Entrypoint for the extension.
 */
void _PG_init(void) {
    next_copydir_hook = copydir_hook;
    copydir_hook = intercept_copydir;
}
}
