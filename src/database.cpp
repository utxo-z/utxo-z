// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file database.cpp
 * @brief Database interface implementation
 */

#include <utxoz/database.hpp>
#include "detail/database_impl.hpp"

namespace utxoz {

db::db() = default;
db::~db() = default;
db::db(db&&) noexcept = default;
db& db::operator=(db&&) noexcept = default;

void db::configure(std::string_view path, bool remove_existing) {
    if (!impl_) {
        impl_ = std::make_unique<detail::database_impl>();
    }
    impl_->configure(path, remove_existing);
}

void db::configure_for_testing(std::string_view path, bool remove_existing) {
    if (!impl_) {
        impl_ = std::make_unique<detail::database_impl>();
    }
    impl_->configure_for_testing(path, remove_existing);
}

void db::close() {
    if (impl_) {
        impl_->close();
    }
}

size_t db::size() const {
    return impl_ ? impl_->size() : 0;
}

bool db::insert(raw_outpoint const& key, output_data_span value, uint32_t height) {
    if (!impl_) {
        throw std::runtime_error("Database not configured");
    }
    return impl_->insert(key, value, height);
}

bytes_opt db::find(raw_outpoint const& key, uint32_t height) const {
    if (!impl_) {
        return std::nullopt;
    }
    return impl_->find(key, height);
}

size_t db::erase(raw_outpoint const& key, uint32_t height) {
    if (!impl_) {
        return 0;
    }
    return impl_->erase(key, height);
}

std::pair<uint32_t, std::vector<deferred_deletion_entry>> db::process_pending_deletions() {
    if (!impl_) {
        return {0, {}};
    }
    return impl_->process_pending_deletions();
}

size_t db::deferred_deletions_size() const {
    return impl_ ? impl_->deferred_deletions_size() : 0;
}

std::pair<flat_map<raw_outpoint, bytes>, std::vector<deferred_lookup_entry>> db::process_pending_lookups() {
    if (!impl_) {
        return {};
    }
    return impl_->process_pending_lookups();
}

size_t db::deferred_lookups_size() const {
    return impl_ ? impl_->deferred_lookups_size() : 0;
}

void db::compact_all() {
    if (impl_) {
        impl_->compact_all();
    }
}

database_statistics db::get_statistics() {
    if (!impl_) {
        return {};
    }
    return impl_->get_statistics();
}

void db::print_statistics() {
    if (impl_) {
        impl_->print_statistics();
    }
}

void db::reset_all_statistics() {
    if (impl_) {
        impl_->reset_all_statistics();
    }
}

search_stats const& db::get_search_stats() const {
    static search_stats empty_stats;
    return impl_ ? impl_->get_search_stats() : empty_stats;
}

void db::reset_search_stats() {
    if (impl_) {
        impl_->reset_search_stats();
    }
}

float db::get_cache_hit_rate() const {
    return impl_ ? impl_->get_cache_hit_rate() : 0.0f;
}

std::vector<std::pair<size_t, size_t>> db::get_cached_file_info() const {
    return impl_ ? impl_->get_cached_file_info() : std::vector<std::pair<size_t, size_t>>{};
}

} // namespace utxoz
