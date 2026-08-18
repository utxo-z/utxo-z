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
 *
 * Every include here comes from utxoz or from the standard library, and that is
 * the point rather than a coincidence. This file used to print with
 * `fmt::println`, and CMakeLists.txt declares one package: utxoz. On Linux and
 * macOS the include resolved anyway, through fmt reaching the compiler's search
 * path as a dependency of utxoz rather than as anything this consumer asked
 * for; on Windows it did not, and the 0.9.1 release verification failed to
 * compile a package that was published, retrievable and correct (#113).
 *
 * A package whose headers are pulled in without being declared is a package
 * this test cannot speak about: it would pass or fail on how utxoz happens to
 * propagate its own dependencies, which is not what it is here to measure. fmt
 * is an implementation dependency — no header under include/utxoz includes it —
 * so the printing moved to <iostream>.
 *
 * <iostream> rather than <print> or <format>, deliberately. This consumer is
 * compiled by whatever toolchain a release runs on, and a diagnostic that fails
 * because the compiler's C++23 library is incomplete would report utxoz as
 * broken when it is not. Stream insertion is the one output facility every
 * implementation that can build this package already has.
 */

#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <span>

#include <utxoz/config.hpp>
#include <utxoz/database.hpp>
#include <utxoz/version.hpp>

int main() {
    std::cout << "utxoz " << utxoz::version << '\n';

#if UTXOZ_STATISTICS_LEVEL >= 1
    std::cout << "statistics: enabled\n";
#else
    std::cout << "statistics: disabled\n";
#endif

    if (utxoz::version.empty()) {
        std::cout << "the packaged version.hpp carries no version\n";
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

        auto opened = utxoz::full_db::open_for_testing(path, true);
        if ( ! opened) {
            std::cout << "full_db: open failed\n";
            return EXIT_FAILURE;
        }

        auto db = std::move(*opened);
        utxoz::raw_outpoint key{};
        key[0] = 1;

        std::array<uint8_t, 32> const value{};
        if ( ! db.insert(key, utxoz::output_data_span{value}, 100)) {
            std::cout << "full_db: insert failed\n";
            return EXIT_FAILURE;
        }

        auto const found = db.find(key, 200);
        if ( ! found) {
            std::cout << "full_db: find failed\n";
            return EXIT_FAILURE;
        }
        if (found->block_height != 100) {
            std::cout << "full_db: find returned the wrong height\n";
            return EXIT_FAILURE;
        }

        db.close();
        std::filesystem::remove_all(path);
        std::cout << "full_db: ok\n";
    }

    {
        auto const path = root / "utxoz_test_package_reference";
        std::filesystem::remove_all(path);

        auto opened = utxoz::reference_db::open_for_testing(path, true);
        if ( ! opened) {
            std::cout << "reference_db: open failed\n";
            return EXIT_FAILURE;
        }

        auto db = std::move(*opened);
        utxoz::raw_outpoint key{};
        key[0] = 2;

        if ( ! db.insert(key, 7, 42, 100)) {
            std::cout << "reference_db: insert failed\n";
            return EXIT_FAILURE;
        }

        auto const found = db.find(key, 200);
        if ( ! found) {
            std::cout << "reference_db: find failed\n";
            return EXIT_FAILURE;
        }
        // The typed fields come back, which is the whole point of this mode and
        // the part that would break if reference_find_result ever stopped being
        // installed.
        if (found->file_number != 7 || found->offset != 42) {
            std::cout << "reference_db: find returned the wrong fields\n";
            return EXIT_FAILURE;
        }

        db.close();
        std::filesystem::remove_all(path);
        std::cout << "reference_db: ok\n";
    }

    std::cout << "ok\n";
    return EXIT_SUCCESS;
}
