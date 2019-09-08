#include <cstdio>
#include <filesystem>
#include <fstream>

#include <cxxopts.hpp>
#include <libzfs.h>
#include <spdlog/spdlog.h>

#include <pgcow/fs.h>
#include <pgcow/postgres/data_directory.h>
#include <pgcow/version.h>
#include <pgcow/zfs/dataset.h>

int main(int argc, char **argv) {
    spdlog::set_level(spdlog::level::info);

    cxxopts::Options options("pgcow-initdb",
                             "PGCow InitDB\n");
    options.positional_help("[zdataset]")
        .show_positional_help()
        .add_options()("zdataset",
                       "Name of a ZFS dataset to use as a data directory",
                       cxxopts::value<std::string>())(
            "h,help", "Prints a list of options");

    options.parse_positional({"zdataset"});

    std::string dataset_name;

    try {
        auto result = options.parse(argc, argv);
        if (result.count("help") || result.arguments().size() == 0) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        dataset_name = result["zdataset"].as<std::string>();
    } catch (const cxxopts::option_not_exists_exception &err) {
        std::cout << options.help() << std::endl;
        return 1;
    }

    auto zfs = libzfs_init();
    if (!zfs) {
        spdlog::critical("could not initialize libzfs");
        return 1;
    }

    auto dataset = pgcow::zfs::dataset::by_name(zfs, dataset_name);
    if (!dataset) {
        spdlog::critical("cannot find zfs dataset named '{0}'", dataset_name);
        return 1;
    }

    spdlog::info("zfs dataset '{0}' is mounted at {1}", dataset->name(),
                 dataset->mountpoint());

    if (pgcow::postgres::data_directory::is(dataset->mountpoint())) {
        spdlog::critical(
            "{0} does not appear to be a data directory, run 'initdb -D {0}'",
            dataset->mountpoint());
    }

    spdlog::debug("'{0}' appears to be a valid pg data directory",
                  dataset->mountpoint());
    auto data_directory =
        pgcow::postgres::data_directory(dataset->mountpoint());

    if (data_directory.has_tablespaces()) {
        spdlog::critical(
            "'{0}' has custom tablespaces, these are not supported by pgcow",
            dataset->mountpoint());
        return 1;
    }

    spdlog::debug("'{0}' does not have custom tablespaces",
                  dataset->mountpoint());

    if (data_directory.is_running()) {
        spdlog::critical("a pg server is running and is using '{0}' as a data "
                         "directory, stop the server and try again",
                         dataset->mountpoint());
        return 1;
    }

    auto data_directory_paths = data_directory.paths();
    if (std::filesystem::exists(data_directory_paths.pgcow_version)) {
        spdlog::critical("pgcow already initialized in '{0}'",
                         dataset->mountpoint());
        return 1;
    }

    auto databases_directory = data_directory_paths.databases;
    auto databases_dataset_name = pgcow::fs::path::leaf(databases_directory);

    spdlog::debug("transforming pg databases directory '{0}' into zfs dataset",
                  databases_directory);

    auto databases_dataset =
        pgcow::zfs::dataset::create(zfs, dataset, databases_dataset_name);
    if (!databases_dataset) {
        spdlog::critical("failed to transform '{0}' into zfs dataset",
                         databases_directory);
        return 1;
    }

    spdlog::info("transformed pg databases directory '{0}' into a zfs dataset "
                 "named '{1}'",
                 databases_directory, databases_dataset->name());

    for (const auto &database_directory :
         data_directory.database_directories()) {
        auto [oid, path] = database_directory;
        spdlog::debug("tranforming pg database directory '{0}' (oid: {1}) into "
                      "zfs dataset",
                      path, oid);

        auto db_dataset =
            pgcow::zfs::dataset::create(zfs, databases_dataset, oid);
        if (!db_dataset) {
            spdlog::critical("failed to transform '{0}' into a zfs dataset",
                             path);
            return 1;
        }

        spdlog::info("transformed pg database directory '{0} (oid: {1}) into a "
                     "zfs dataset",
                     path, oid);
    }

    spdlog::debug("writing pgcow version file to '{0}'",
                  data_directory_paths.pgcow_version);

    try {
        std::ofstream version_file(data_directory_paths.pgcow_version);
        version_file << pgcow::version;
        version_file.close();
    } catch (const std::ofstream::failure &err) {
        spdlog::error("failed to write pgcow version file to '{0}'",
                      data_directory_paths.pgcow_version);
        return 1;
    }

    spdlog::info("pgcow v{0} initialized in zfs data set '{1}' ('{2}')",
                 pgcow::version, dataset_name, dataset->mountpoint());
    return 0;
}
