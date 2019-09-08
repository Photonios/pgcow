#include <memory>
#include <vector>
#include <string>

#include <libzfs.h>

namespace pgcow {
namespace zfs {

/**
 * Gets a string describing the last error that occurred as a result
 * of calling libzfs functions.
 */
std::string error_description();

/**
 * Wraps zfs_* related function and provides RAII.
 */
class dataset {
  public:
    /**
     * Creates a new \see dataset from \see zfs_handle_t*
     */
    explicit dataset(zfs_handle_t *handle) noexcept;

    /**
     * Closes/frees the underlying zfs_handle_t*.
     */
    ~dataset() noexcept;

    /**
     * Deletes the ZFS dataset with the specified name.
     *
     * \param handle             ZFS Dataset handle pointing to the ZFS dataset
     *                           to destroy/delete.
     * \param delete_mountpoint  Whether to also delete the directory in which
     *                           the dataset was mounted (if any).
     */
    static void destroy(zfs_handle_t *handle, bool delete_mountpoint = false);

    /**
     * Opens a ZFS dataset with the specified name.
     *
     * \param zfs  ZFS Library handle.
     * \param name Name of the dataset to open.
     *
     * \returns An instance of \see dataset or nullptr if no such
     *          dataset can be found.
     */
    static std::shared_ptr<dataset> by_name(libzfs_handle_t *zfs,
                                            const std::string &name);

    /**
     * Opens a ZFS dataset by its mountpoint.
     *
     * \param zfs        ZFS Library handle.
     * \param mountpoint Absolute path to a ZFS dataset mountpoint.
     *
     * \returns An instance of \see dataset or nullptr if no dataset
     *          is mounted at the specified mountpoint.
     */
    static std::shared_ptr<dataset>
    by_mountpoint(libzfs_handle_t *zfs, const std::string &mountpoint);

    /**
     * Creates a new ZFS dataset with the specified name
     * and mounts it at the specified path.
     *
     * \param zfs            ZFS Libary Handle.
     * \param parent_dataset Dataset the new dataset should become
     *                       a child of.
     * \param name           Name of the dataset to create. This should
     *                       not include the name of the parent/ancestor
     *                       dataset(s).
     *
     * \returns An instance of \see dataset, representing the newly
     *          created dataset. Nullptr if creation of the dataset
     *          failed.
     */
    static std::shared_ptr<dataset>
    create(libzfs_handle_t *zfs, std::shared_ptr<dataset> parent_dataset,
           const std::string &name);

    /**
     * Gets a list of all available datasets.
     */
    static std::vector<std::shared_ptr<dataset>> all(libzfs_handle_t *zfs);

    /**
     * Gets the name of this dataset.
     */
    std::string name() const;

    /**
     * Gets the absolute path to where this dataset is mounted.
     */
    std::string mountpoint() const;

    /**
     * Creates a snapshot of this dataset with the specified name.
     *
     * \param name The name to give to the snapshot.
     *
     * \returns An instance of \see dataset, representing the newly
     *          created snapshot. Nullptr if creation of the dataset
     *          failed.
     */
    std::shared_ptr<dataset> snapshot(const std::string &name) const;

    /**
     * Creates a clone of this snapshot with the speciifed name.
     *
     * \param name The name to use for the new, cloned dataset. This
     *             should not include the name of the parent dataset.
     *
     * \returns An instance of \see dataset, representing the newly
     *          created clone. Nullptr if creation of the dataset
     *          failed.
     */
    std::shared_ptr<dataset> clone(const std::string &name) const;

  private:
    /**
     * Iterates over all datasets and appends them to \paramref datasets.
     */
    static void iterate(libzfs_handle_t *zfs,
                        std::vector<std::shared_ptr<dataset>> &datasets);

    /**
     * Iterates over all child datasets and appends them to \paramref datases.
     */
    static void iterate(zfs_handle_t *handle,
                        std::vector<std::shared_ptr<dataset>> &datasets);

  private:
    zfs_handle_t *handle_ = nullptr;
};

} // namespace zfs
} // namespace pgcow
