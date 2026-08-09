// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * A consumer of the packaged library, no smaller than it has to be.
 *
 * The two generated headers are included by name and then actually used, because
 * an unused include can be optimised into nothing by a compiler that never opens
 * it. `version` is printed and `config.hpp` is read through a preprocessor
 * branch, so both files have to exist and have to have content.
 *
 * A database is opened in each storage mode as well: a package whose headers
 * install but whose library does not link is just as unusable, and a package
 * whose full_db links while reference_db does not is broken for half its users.
 * Both failures belong here rather than downstream.
 */

#include <array>
#include <cstdlib>
#include <filesystem>
#include <span>

#include <fmt/format.h>

#include <utxoz/config.hpp>
#include <utxoz/database.hpp>
#include <utxoz/version.hpp>

int main() {
    fmt::println("utxoz {}", utxoz::version);

#ifdef UTXOZ_STATISTICS_ENABLED
    fmt::println("statistics: enabled");
#else
    fmt::println("statistics: disabled");
#endif

    if (utxoz::version.empty()) {
        fmt::println("the packaged version.hpp carries no version");
        return EXIT_FAILURE;
    }

    // The working directory, not the system temp directory. Conan runs each test
    // package in its own build tree, so this is already isolated; a fixed path
    // under /tmp is shared, and two `conan create` runs at once would open or
    // delete each other's database.
    auto const root = std::filesystem::current_path();

    // Both storage modes, because they are separate entry points in the
    // installed library. A package whose full_db links while reference_db does
    // not is still broken for anyone using the reference mode, and that failure
    // would surface downstream rather than here.
    {
        auto const path = root / "utxoz_test_package_full";
        std::filesystem::remove_all(path);

        auto opened = utxoz::full_db::open_for_testing(path.string(), true);
        if ( ! opened) {
            fmt::println("full_db: open failed");
            return EXIT_FAILURE;
        }

        auto db = std::move(*opened);
        utxoz::raw_outpoint key{};
        key[0] = 1;

        std::array<uint8_t, 32> const value{};
        if ( ! db.insert(key, utxoz::output_data_span{value}, 100)) {
            fmt::println("full_db: insert failed");
            return EXIT_FAILURE;
        }

        auto const found = db.find(key, 200);
        if ( ! found) {
            fmt::println("full_db: find failed");
            return EXIT_FAILURE;
        }
        if (found->block_height != 100) {
            fmt::println("full_db: find returned the wrong height");
            return EXIT_FAILURE;
        }

        db.close();
        std::filesystem::remove_all(path);
        fmt::println("full_db: ok");
    }

    {
        auto const path = root / "utxoz_test_package_reference";
        std::filesystem::remove_all(path);

        auto opened = utxoz::reference_db::open_for_testing(path.string(), true);
        if ( ! opened) {
            fmt::println("reference_db: open failed");
            return EXIT_FAILURE;
        }

        auto db = std::move(*opened);
        utxoz::raw_outpoint key{};
        key[0] = 2;

        if ( ! db.insert(key, 7, 42, 100)) {
            fmt::println("reference_db: insert failed");
            return EXIT_FAILURE;
        }

        auto const found = db.find(key, 200);
        if ( ! found) {
            fmt::println("reference_db: find failed");
            return EXIT_FAILURE;
        }
        // The typed fields come back, which is the whole point of this mode and
        // the part that would break if reference_find_result ever stopped being
        // installed.
        if (found->file_number != 7 || found->offset != 42) {
            fmt::println("reference_db: find returned the wrong fields");
            return EXIT_FAILURE;
        }

        db.close();
        std::filesystem::remove_all(path);
        fmt::println("reference_db: ok");
    }

    fmt::println("ok");
    return EXIT_SUCCESS;
}
