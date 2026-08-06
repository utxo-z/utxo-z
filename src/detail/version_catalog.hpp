// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file version_catalog.hpp
 * @brief The version files a container has, and the metadata describing them.
 * @internal
 *
 * A version number is an identity, not a position. Files are never renumbered,
 * so the numbering has holes in it wherever compaction has drained one, and it
 * only ever climbs: after a long run the live files may be v900 and v901 with
 * nothing below them. Nothing may therefore be sized or iterated by the highest
 * number — that would grow forever while the database itself does not.
 *
 * The catalogue is the one place that knows which versions exist. It is built
 * once, when the database opens, and kept current as versions are created and
 * removed. Every traversal, every metadata lookup and compaction itself go
 * through it, so no operation pays for a directory scan or for the gaps.
 */

#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <optional>
#include <system_error>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>
#include <fmt/format.h>

#include <utxoz/types.hpp>

#include "file_metadata.hpp"

namespace utxoz::detail {

namespace fs = std::filesystem;

/**
 * @brief The versions present for one container, ascending, and their metadata.
 *
 * Sparse in both: the version list holds only what exists, and metadata is keyed
 * by version rather than indexed by it.
 */
class version_catalog {
public:
    /// Versions present, ascending. Never contains a duplicate.
    [[nodiscard]]
    std::vector<size_t> const& versions() const noexcept { return versions_; }

    [[nodiscard]]
    bool empty() const noexcept { return versions_.empty(); }

    [[nodiscard]]
    size_t size() const noexcept { return versions_.size(); }

    /// The active version: the highest one present. Zero when empty, which is
    /// the version a fresh database creates first.
    [[nodiscard]]
    size_t active() const noexcept { return versions_.empty() ? 0 : versions_.back(); }

    [[nodiscard]]
    bool contains(size_t version) const noexcept {
        return std::ranges::binary_search(versions_, version);
    }

    /// Records a version. Idempotent, and keeps the list ordered.
    void add(size_t version) {
        highest_issued_ = std::max(highest_issued_, version);
        auto const pos = std::ranges::lower_bound(versions_, version);
        if (pos != versions_.end() && *pos == version) return;
        versions_.insert(pos, version);
    }

    /// The identity the next version must take.
    ///
    /// One past the highest ever issued, not one past the highest still here:
    /// compaction can remove the top of the range, and reissuing a number it
    /// just retired would give a new file the name an old one had. Anything
    /// still keyed by that pair — the file cache, a reader mid-resolution —
    /// would then be pointing at the wrong file rather than at a missing one.
    /// Across a restart the mark is rebuilt from what is on disk, where no such
    /// reference can survive.
    [[nodiscard]]
    size_t next_version() const noexcept {
        return versions_.empty() && highest_issued_ == 0 ? 0 : highest_issued_ + 1;
    }

    /// Drops a version and the metadata describing it.
    void remove(size_t version) {
        auto const pos = std::ranges::lower_bound(versions_, version);
        if (pos != versions_.end() && *pos == version) versions_.erase(pos);
        metadata_.erase(version);
    }

    void clear() noexcept {
        versions_.clear();
        metadata_.clear();
        highest_issued_ = 0;
    }

    /// The versions below `version`, highest first: the order a historical
    /// resolution walks them, nearest generation first.
    [[nodiscard]]
    std::vector<size_t> below(size_t version) const {
        std::vector<size_t> out;
        auto const end = std::ranges::lower_bound(versions_, version);
        out.assign(versions_.begin(), end);
        std::ranges::reverse(out);
        return out;
    }

    /// Metadata for a version, creating an empty record if there is none.
    /// Only meaningful for a version in the catalogue.
    [[nodiscard]]
    file_metadata& metadata(size_t version) { return metadata_[version]; }

    /// Metadata for a version, or nullptr when nothing describes it. Callers
    /// must treat nullptr as "unknown", never as "empty" — an absent record
    /// says nothing about the file's contents and must not be used to skip it.
    [[nodiscard]]
    file_metadata const* find_metadata(size_t version) const {
        auto const it = metadata_.find(version);
        return it == metadata_.end() ? nullptr : &it->second;
    }

    [[nodiscard]]
    file_metadata* find_metadata(size_t version) {
        auto const it = metadata_.find(version);
        return it == metadata_.end() ? nullptr : &it->second;
    }

    void erase_metadata(size_t version) { metadata_.erase(version); }

    void clear_metadata() noexcept { metadata_.clear(); }

private:
    std::vector<size_t> versions_;
    boost::unordered_flat_map<size_t, file_metadata> metadata_;
    size_t highest_issued_ = 0;
};

/**
 * @brief Does this path exist?
 *
 * The throwing overload of fs::exists reports a permission failure by throwing,
 * and an exception escaping open() is not something a caller of a result-typed
 * API is looking for. This distinguishes the three answers that matter: it is
 * there, it is not there, or we could not tell — and the last one is an error,
 * never a "no".
 */
[[nodiscard]]
inline result<bool> path_exists(fs::path const& p) {
    std::error_code ec;
    auto const status = fs::status(p, ec);
    if (status.type() == fs::file_type::not_found) return false;
    if (ec) return std::unexpected(error_code::catalog_unreadable);
    return true;
}

/**
 * @brief Reads the version number out of a canonical file name.
 *
 * Canonical means the name this build would produce for that version and no
 * other spelling of it: `cont_0_v00007.dat`, not `cont_0_v7.dat` and not
 * `cont_0_v000007.dat`. The check is done by reformatting the parsed number and
 * requiring the result to match, so an alias cannot enter the catalogue and be
 * opened later under a name that no longer resolves to the same file.
 */
[[nodiscard]]
inline std::optional<size_t> parse_canonical_version(std::string const& name,
                                                     std::string const& prefix) {
    static constexpr std::string_view suffix = ".dat";

    if (name.size() <= prefix.size() + suffix.size()) return std::nullopt;
    if (name.compare(0, prefix.size(), prefix) != 0) return std::nullopt;
    if (name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) return std::nullopt;

    auto const digits = name.substr(prefix.size(), name.size() - prefix.size() - suffix.size());
    if (digits.empty() || digits.size() > 20) return std::nullopt;
    if (digits.find_first_not_of("0123456789") != std::string::npos) return std::nullopt;

    size_t value = 0;
    try {
        value = std::stoull(digits);
    } catch (std::exception const&) {
        return std::nullopt;   // out of range for size_t
    }

    // Round-trips to the same name, or it is not the name we would have written.
    if (fmt::format("{:05}", value) != digits) return std::nullopt;
    return value;
}

/**
 * @brief Version numbers of the regular files in `dir` matching `prefix`.
 *
 * Fail-closed: any error reading the directory is returned, never swallowed. A
 * catalogue that cannot be read is not an empty catalogue — reporting one as
 * the other is how a database with versions in it gets opened as if it had
 * none, and then written over. Callers must propagate the failure rather than
 * proceed on a partial list; there is no partial list to proceed on, because
 * nothing is published unless the whole directory was walked.
 */
[[nodiscard]]
inline result<std::vector<size_t>> enumerate_versions(fs::path const& dir, std::string const& prefix) {
    std::error_code ec;

    auto const status = fs::status(dir, ec);

    // Absent is a real answer — a database that has not been created yet has no
    // versions. Anything else that goes wrong is not an answer at all.
    if (status.type() == fs::file_type::not_found) return std::vector<size_t>{};
    if (ec) return std::unexpected(error_code::catalog_unreadable);
    if ( ! fs::is_directory(status)) return std::unexpected(error_code::catalog_unreadable);

    fs::directory_iterator it(dir, ec);
    if (ec) return std::unexpected(error_code::catalog_unreadable);

    // The increment is checked where it happens, not at the top of the next
    // iteration. A failing increment can leave the iterator at the end, and then
    // the loop simply finishes and a check placed in the body never runs — which
    // would return everything read so far as if it were the whole directory.
    // That is the partial list this function promises not to publish.
    std::vector<size_t> versions;
    auto const end = fs::directory_iterator{};
    while (it != end) {
        // A directory or a symlink to one must not be catalogued as a version
        // file; is_regular_file follows symlinks, which is what we want for the
        // file itself.
        auto const entry_status = it->status(ec);
        if (ec) return std::unexpected(error_code::catalog_unreadable);

        if (fs::is_regular_file(entry_status)) {
            if (auto const v = parse_canonical_version(it->path().filename().string(), prefix)) {
                versions.push_back(*v);
            }
        }

        it.increment(ec);
        if (ec) return std::unexpected(error_code::catalog_unreadable);
    }

    std::ranges::sort(versions);
    versions.erase(std::ranges::unique(versions).begin(), versions.end());
    return versions;
}

} // namespace utxoz::detail
