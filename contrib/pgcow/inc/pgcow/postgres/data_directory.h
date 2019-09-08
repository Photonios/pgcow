#include <string>
#include <tuple>
#include <vector>

namespace pgcow {
namespace postgres {

/**
 * Absolute paths to files and directories in the data directory.
 */
struct data_directory_paths {
    std::string config;
    std::string databases;
    std::string tablespaces;
    std::string pid;
    std::string pgcow_version;
};

/**
 * Represents an initialized PostgreSQL data directory.
 */
class data_directory {
  public:
    /**
     * Creates a new \see data_directory instance.
     *
     * \param path Absolute path to the root of the PostgreSQL
     *             data directory.
     */
    explicit data_directory(const std::string &path) noexcept;

    /**
     * Gets a structure with sub paths in the data directory.
     */
    data_directory_paths paths() const;

    /**
     * Gets a list of directories, where each directory is a database.
     *
     * \returns A tuple of directory names and the absolute path
     *          to the directory.
     *
     *          Directory names are database OID's.
     */
    std::vector<std::tuple<std::string, std::string>>
    database_directories() const;

    /**
     * Gets a list of symlinks, where each symlink is to a
     * tablespace directory.
     */
    std::vector<std::string> tablespace_symlinks() const;

    /**
     * Gets whether there are custom tablespaces defined.
     */
    bool has_tablespaces() const;

    /**
     * Gets whether there is a PostgreSQL server running
     * using this data directory.
     */
    bool is_running() const;

    /**
     * Gets whether the specified path is a valid PostgreSQL
     * directory.
     *
     * This is not conclusive, but it can definitely tell you
     * if its not.
     */
    static bool is(const std::string &path) noexcept;

  private:
    /**
     * Absolute path to the root of the PostgreSQL data directory.
     */
    std::string path_;
};

} // namespace postgres
} // namespace pgcow
