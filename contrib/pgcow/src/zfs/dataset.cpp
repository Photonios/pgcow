#include <filesystem>
#include <memory>
#include <string>

#include <libzfs.h>
#include <spdlog/spdlog.h>

#include <pgcow/fs.h>
#include <pgcow/zfs/dataset.h>
#include <pgcow/zfs/error.h>

namespace pgcow {
namespace zfs {

/**
 * Passed to zfs_iter_* functions to append results to \paramref data.
 */
static int __iterate_datasets(zfs_handle_t *handle, void *data) {
    std::vector<std::shared_ptr<dataset>> *datasets =
        reinterpret_cast<std::vector<std::shared_ptr<dataset>> *>(data);
    datasets->push_back(std::make_shared<dataset>(handle));
    return 0;
}

dataset::dataset(zfs_handle_t *handle) noexcept : handle_(handle) {}

dataset::~dataset() noexcept {
    if (this->handle_) {
        zfs_close(this->handle_);
    }
}

void dataset::destroy(zfs_handle_t *handle, bool delete_mountpoint) {
    if (!handle) {
        return;
    }

    dataset ds(handle);
    auto mountpoint = ds.mountpoint();

    zfs_destroy(handle, B_FALSE);

    if (delete_mountpoint && mountpoint != "") {
        std::error_code _;
        std::filesystem::remove_all(mountpoint, _);
    }
}

std::shared_ptr<dataset> dataset::by_name(libzfs_handle_t *zfs,
                                          const std::string &name) {
    zfs_handle_t *handle = zfs_open(zfs, name.c_str(), ZFS_TYPE_DATASET);
    if (!handle) {
        return nullptr;
    }

    return std::make_shared<dataset>(handle);
}

std::vector<std::shared_ptr<dataset>> dataset::all(libzfs_handle_t *zfs) {
    std::vector<std::shared_ptr<dataset>> datasets;
    iterate(zfs, datasets);

    return datasets;
}

std::shared_ptr<dataset> dataset::by_mountpoint(libzfs_handle_t *zfs,
                                                const std::string &mountpoint) {

    std::vector<std::shared_ptr<dataset>> datasets = all(zfs);

    for (auto dataset : datasets) {
        if (dataset->mountpoint() == mountpoint) {
            return dataset;
        }
    }

    return nullptr;
}

std::shared_ptr<dataset>
dataset::create(libzfs_handle_t *zfs, std::shared_ptr<dataset> parent_dataset,
                const std::string &name) {

    if (!parent_dataset) {
        spdlog::error("cannot create dataset '{0}' without a parent", name);
        return nullptr;
    }

    std::string dataset_name = parent_dataset->name() + "/" + name;
    std::string dataset_mountpoint =
        pgcow::fs::path::join(parent_dataset->mountpoint(), name);

    // creating a dataset in a directory that already exists
    // is supported, we just need to move the data out of the
    // way, create the dataset and then move it back because ZFS
    // wants an empty/non-existent dir to create the dataset
    if (std::filesystem::exists(dataset_mountpoint)) {
        if (!std::filesystem::is_directory(dataset_mountpoint)) {
            spdlog::error("cannot create zfs dataset '{0}', mountpoint '{1}' "
                          "is not a directory",
                          dataset_name, dataset_mountpoint);
            return nullptr;
        }

        // rename the target mount point to *.bak
        std::string dataset_mountpoint_temp = dataset_mountpoint + ".bak";
        spdlog::debug("renaming '{0}' to '{1}' to create zfs dataset '{2}'",
                      dataset_mountpoint, dataset_mountpoint_temp,
                      dataset_name);

        try {
            std::filesystem::rename(dataset_mountpoint,
                                    dataset_mountpoint_temp);
        } catch (const std::filesystem::filesystem_error &err) {
            spdlog::error("cannot create zfs dataset '{0}', cannot rename "
                          "'{1}' to '{2}', error {3}",
                          dataset_name, dataset_mountpoint,
                          dataset_mountpoint_temp, err.what());
            return nullptr;
        }

        spdlog::debug(
            "deleting target mountpoint '{0}' to create zfs dataset '{1}'",
            dataset_mountpoint, dataset_name);

        try {
            std::filesystem::remove_all(dataset_mountpoint);
        } catch (const std::filesystem::filesystem_error &err) {
            spdlog::error("cannot create zfs dataset '{0}', cannot delete "
                          "'{1}', error {2}",
                          dataset_name, dataset_mountpoint, err.what());

            // rename the target mountpoint back so everything is as it was
            std::error_code _;
            std::filesystem::rename(dataset_mountpoint_temp, dataset_mountpoint,
                                    _);
            spdlog::debug("creating zfs dataset '{0}' failed, renaming target "
                          "mountpoint back from '{0}' to '{1}'",
                          dataset_name, dataset_mountpoint_temp,
                          dataset_mountpoint);

            return nullptr;
        }

        spdlog::debug("creating zfs dataset '{0}' in '{1}", dataset_name,
                      dataset_mountpoint);

        // create the dataset with the specified name
        int err = zfs_create(zfs, dataset_name.c_str(), ZFS_TYPE_FILESYSTEM, 0);
        if (err != 0) {
            spdlog::error("failed to create zfs dataset '{0}', error {1}", name,
                          error_description(zfs));

            // rename the target mountpoint back so everything is as it was
            std::error_code _;
            std::filesystem::rename(dataset_mountpoint_temp, dataset_mountpoint,
                                    _);
            spdlog::debug("creating zfs dataset '{0}' failed, renaming target "
                          "mountpoint back from '{0}' to '{1}'",
                          dataset_name, dataset_mountpoint_temp,
                          dataset_mountpoint);

            return nullptr;
        }

        // open up the dataset so we can make sure all is good
        auto dataset = dataset::by_name(zfs, dataset_name);
        if (!dataset) {
            // this is really bad and we can't recover... the filesystem
            // is fucked.. we can't nuke the dataset without being able
            // to open it.. so we can't even put everything back...
            // no data is lost, but one needs to manually nuke the dataset
            // and rename the target mountpoint back
            spdlog::critical(
                "created zfs dataset '{0}', but cannot open, filesystem is now "
                "in a inconsistent state, error {1}",
                dataset_name, error_description(zfs));
            return nullptr;
        }

        // make sure it got mounted in the right place
        if (dataset->mountpoint() != dataset_mountpoint) {
            spdlog::error("created zfs dataset '{0}', but got mounted in '{1}' "
                          "instead of '{2}'",
                          dataset_name, dataset->mountpoint(),
                          dataset_mountpoint);

            // destroy the dataset to back to how all was before
            dataset::destroy(dataset->handle_);

            // rename the target mountpoint back so everything is as it was
            std::error_code _;
            std::filesystem::rename(dataset_mountpoint_temp, dataset_mountpoint,
                                    _);
            spdlog::debug("creating zfs dataset '{0}' failed, renaming target "
                          "mountpoint back from '{0}' to '{1}'",
                          dataset_name, dataset_mountpoint_temp,
                          dataset_mountpoint);

            return nullptr;
        }

        // rename the target mountpoint so files end up right where they were
        spdlog::debug(
            "renaming '{0}' to '{1}' for creation of zfs data set '{2}'",
            dataset_mountpoint_temp, dataset_mountpoint, dataset_name);

        try {
            std::filesystem::rename(dataset_mountpoint_temp,
                                    dataset_mountpoint);
        } catch (const std::filesystem::filesystem_error &err) {
            spdlog::critical("created zfs dataset '{0}', but cannot restore "
                             "mountpoint '{1}' from '{2}', filesystem is now "
                             "in a inconsistent state, error {3}",
                             dataset_name, dataset_mountpoint,
                             dataset_mountpoint_temp, err.what());
            return nullptr;
        }

        return dataset;
    }

    // mountpoint doesn't exists already.. this is simple...
    spdlog::debug("creating zfs dataset '{0}' in '{1}'", dataset_name,
                  dataset_mountpoint);

    int err = zfs_create(zfs, dataset_name.c_str(), ZFS_TYPE_FILESYSTEM, 0);
    if (err != 0) {
        spdlog::error("failed to create zfs dataset '{0}' in '{1}', error {2}",
                      dataset_name, dataset_mountpoint, error_description(zfs));
        return nullptr;
    }

    auto dataset = dataset::by_name(zfs, name);
    if (!dataset) {
        spdlog::error(
            "created zfs dataset '{0}', but cannot open it, error {1}", name,
            error_description(zfs));
        return nullptr;
    }

    return dataset;
}

std::string dataset::name() const {
    const char *name_ptr = zfs_get_name(this->handle_);

    std::string name;
    name.assign(name_ptr, ::strlen(name_ptr));

    return name;
}

std::string dataset::mountpoint() const {
    std::string mountpoint(ZFS_MAXPROPLEN, ' ');
    int err = zfs_prop_get(this->handle_, ZFS_PROP_MOUNTPOINT, &mountpoint[0],
                           ZFS_MAXPROPLEN, nullptr, nullptr, 0, B_FALSE);

    if (err != 0) {
        spdlog::error("failed to get zfs dataset '{0}' mountpoint, error {2}",
                      this->name(),
                      error_description(zfs_get_handle(this->handle_)));
        return "";
    }

    mountpoint.resize(::strlen(mountpoint.c_str()));
    return mountpoint;
}

std::shared_ptr<dataset> dataset::snapshot(const std::string &name) const {
    std::string snapshot_name = this->name() + "@" + name;

    spdlog::debug("snapshotting zfs dataset '{0}' to '{1}'", this->name(),
                  snapshot_name);

    int err = zfs_snapshot(zfs_get_handle(this->handle_), snapshot_name.c_str(),
                           B_TRUE, 0);
    if (err != 0) {
        spdlog::error("failed to snapshot zfs dataset '{0}', error {1}",
                      snapshot_name,
                      error_description(zfs_get_handle(this->handle_)));
        return nullptr;
    }

    zfs_handle_t *handle = zfs_open(zfs_get_handle(this->handle_),
                                    snapshot_name.c_str(), ZFS_TYPE_SNAPSHOT);
    if (!handle) {
        spdlog::error(
            "snapshotted zfs dataset '{0}', but can't open, error {1}",
            snapshot_name, error_description(zfs_get_handle(this->handle_)));
        return nullptr;
    }

    return std::make_shared<dataset>(handle);
}

std::shared_ptr<dataset> dataset::clone(const std::string &name) const {
    std::string parent_name = this->name();

    // this is most likely a snapshot, remove the snapshot name
    auto at_symbol_index = parent_name.find("@");
    if (at_symbol_index > 0) {
        parent_name = parent_name.substr(0, at_symbol_index);
    }

    // build up name of the cloned dataset
    // (parent_name="pgdata/base/1", name="2", result="pgdata/base/2")
    std::string clone_name = parent_name;
    auto last_slash_index = clone_name.rfind("/");
    if (last_slash_index > 0) {
        // +1 so the trailing slash stays
        clone_name = clone_name.substr(0, last_slash_index + 1);
    }

    clone_name += name;

    spdlog::debug("cloning zfs dataset '{0}' to '{1}'", this->name(),
                  clone_name);

    int err = zfs_clone(this->handle_, clone_name.c_str(), 0);
    if (err != 0) {
        spdlog::error("failed to clone zfs dataset '{0}' into '{1}', error {2}",
                      this->name(), clone_name,
                      error_description(zfs_get_handle(this->handle_)));
        return nullptr;
    }

    return by_name(zfs_get_handle(this->handle_), clone_name);
}

void dataset::iterate(libzfs_handle_t *zfs,
                      std::vector<std::shared_ptr<dataset>> &datasets) {
    int err = zfs_iter_root(zfs, __iterate_datasets,
                            reinterpret_cast<void *>(&datasets));
    if (err != 0) {
        spdlog::error("failed to iterate root zfs datasets, error {0}",
                      error_description(zfs));
        return;
    }

    for (auto dataset : datasets) {
        iterate(dataset->handle_, datasets);
    }
}

void dataset::iterate(zfs_handle_t *handle,
                      std::vector<std::shared_ptr<dataset>> &datasets) {
    std::vector<std::shared_ptr<dataset>> temp_datasets;

    int err = zfs_iter_children(handle, __iterate_datasets, &temp_datasets);
    if (err != 0) {
        spdlog::error("failed to iterate child zfs datasets, error {0}",
                      error_description(zfs_get_handle(handle)));
        return;
    }

    datasets.insert(datasets.end(), std::begin(temp_datasets),
                    std::end(temp_datasets));

    for (auto dataset : temp_datasets) {
        iterate(dataset->handle_, datasets);
    }
}

} // namespace zfs
} // namespace pgcow
