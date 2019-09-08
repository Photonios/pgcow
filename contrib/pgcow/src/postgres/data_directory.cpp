#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

#include <spdlog/spdlog.h>

#include <pgcow/fs.h>
#include <pgcow/postgres/data_directory.h>

namespace pgcow {
namespace postgres {

data_directory::data_directory(const std::string &path) noexcept
    : path_(path) {}

data_directory_paths data_directory::paths() const {
    data_directory_paths paths = {
        pgcow::fs::path::join(this->path_, "postgressql.conf"),
        pgcow::fs::path::join(this->path_, "base"),
        pgcow::fs::path::join(this->path_, "pg_tblspc"),
        pgcow::fs::path::join(this->path_, "postmaster.pid"),
        pgcow::fs::path::join(this->path_, "pgcow.version"),
    };

    return paths;
}

std::vector<std::tuple<std::string, std::string>>
data_directory::database_directories() const {
    std::vector<std::tuple<std::string, std::string>> database_directories;

    auto paths = this->paths();
    if (!std::filesystem::is_directory(paths.databases)) {
        spdlog::error("trying to list databases in '{0}', but not a directory",
                      paths.databases);
        return database_directories;
    }

    for (const auto &entry :
         std::filesystem::directory_iterator(paths.databases)) {
        if (!entry.is_directory()) {
            continue;
        }

        auto database_directory_path = entry.path().u8string();
        auto database_directory_name =
            (*std::next(entry.path().end(), -1)).u8string();

        database_directories.push_back(
            std::make_tuple<std::string, std::string>(
                std::move(database_directory_name),
                std::move(database_directory_path)));
    }

    return database_directories;
}

std::vector<std::string> data_directory::tablespace_symlinks() const {
    std::vector<std::string> tablespace_symlinks;

    auto paths = this->paths();
    if (!std::filesystem::is_directory(paths.tablespaces)) {
        spdlog::error(
            "trying to list tablespaces in '{0}', but not a directory",
            paths.tablespaces);
        return tablespace_symlinks;
    }

    for (const auto &entry :
         std::filesystem::directory_iterator(paths.tablespaces)) {
        if (!entry.is_symlink()) {
            continue;
        }

        tablespace_symlinks.push_back(std::move(entry.path().u8string()));
    }

    return tablespace_symlinks;
}

bool data_directory::has_tablespaces() const {
    return this->tablespace_symlinks().size() > 0;
}

bool data_directory::is_running() const {
    auto paths = this->paths();
    return std::filesystem::exists(paths.pid) &&
           (std::filesystem::is_regular_file(paths.pid) ||
            std::filesystem::is_symlink(paths.pid));
}

bool data_directory::is(const std::string &path) noexcept {
    spdlog::trace("checking if '{0}' is a pg data directory", path);

    if (!std::filesystem::exists(path)) {
        return false;
    }

    auto paths = data_directory(path).paths();

    if (!std::filesystem::exists(paths.config)) {
        return false;
    }

    if (!std::filesystem::exists(paths.databases)) {
        return false;
    }

    return true;
}

} // namespace postgres
} // namespace pgcow
