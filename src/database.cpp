// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file database.cpp
 * @brief Database interface implementation — db_base, full_db, compact_db
 */

#include <utxoz/database.hpp>
#include "detail/database_impl.hpp"

namespace utxoz {

// =============================================================================
// db_base
// =============================================================================

db_base::db_base() = default;

db_base::~db_base() {
    close();
}

db_base::db_base(db_base&&) noexcept = default;
db_base& db_base::operator=(db_base&&) noexcept = default;

void db_base::close() {
    if (impl_) {
        impl_->close();
        impl_.reset();
    }
}

size_t db_base::size() const {
    return impl_ ? impl_->size() : 0;
}

size_t db_base::erase(raw_outpoint const& key, uint32_t height) {
    return impl_ ? impl_->erase(key, height) : 0;
}

std::pair<uint32_t, std::vector<deferred_deletion_entry>> db_base::process_pending_deletions() {
    if (!impl_) return {0, {}};
    return impl_->process_pending_deletions();
}

size_t db_base::deferred_deletions_size() const {
    return impl_ ? impl_->deferred_deletions_size() : 0;
}

size_t db_base::deferred_lookups_size() const {
    return impl_ ? impl_->deferred_lookups_size() : 0;
}

result<> db_base::compact_all() {
    if (!impl_) return std::unexpected(error_code::closed);
    return impl_->compact_all();
}

void db_base::for_each_key_impl(void(*cb)(void*, raw_outpoint const&), void* ctx) const {
    if (impl_) impl_->for_each_key_impl(cb, ctx);
}

database_statistics db_base::get_statistics() {
    if (!impl_) return {};
    return impl_->get_statistics();
}

void db_base::print_statistics() {
    if (impl_) impl_->print_statistics();
}

sizing_report db_base::get_sizing_report() const {
    if (!impl_) return {};
    return impl_->get_sizing_report();
}

void db_base::print_sizing_report() const {
    if (impl_) impl_->print_sizing_report();
}

void db_base::print_height_range_stats() const {
    if (impl_) impl_->print_height_range_stats();
}

void db_base::reset_all_statistics() {
    if (impl_) impl_->reset_all_statistics();
}

void db_base::reset_search_stats() {
    if (impl_) impl_->reset_search_stats();
}

float db_base::get_cache_hit_rate() const {
    return impl_ ? impl_->get_cache_hit_rate() : 0.0f;
}

std::vector<std::pair<size_t, size_t>> db_base::get_cached_file_info() const {
    return impl_ ? impl_->get_cached_file_info() : std::vector<std::pair<size_t, size_t>>{};
}

// =============================================================================
// full_db
// =============================================================================

full_db::full_db() = default;
full_db::~full_db() = default;
full_db::full_db(full_db&&) noexcept = default;
full_db& full_db::operator=(full_db&&) noexcept = default;

result<full_db> full_db::open(std::string_view path, bool remove_existing) {
    full_db db;
    db.impl_ = std::make_unique<detail::database_impl>();
    auto r = db.impl_->configure(path, remove_existing, storage_mode::full);
    if (!r) return std::unexpected(r.error());
    return db;
}

result<full_db> full_db::open_for_testing(std::string_view path, bool remove_existing) {
    full_db db;
    db.impl_ = std::make_unique<detail::database_impl>();
    auto r = db.impl_->configure_for_testing(path, remove_existing, storage_mode::full);
    if (!r) return std::unexpected(r.error());
    return db;
}

result<bool> full_db::insert(raw_outpoint const& key, output_data_span value, uint32_t height) {
    if (!impl_) return std::unexpected(error_code::closed);
    return impl_->insert(key, value, height);
}

result<full_find_result> full_db::find(raw_outpoint const& key, uint32_t height) const {
    if (!impl_) return std::unexpected(error_code::closed);
    auto r = impl_->full_find(key, height);
    if (!r) return std::unexpected(error_code::not_found);
    return std::move(*r);
}

std::pair<flat_map<raw_outpoint, full_find_result>, std::vector<deferred_lookup_entry>>
full_db::process_pending_lookups() {
    if (!impl_) return {};
    return impl_->full_process_pending_lookups();
}

void full_db::for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, std::span<uint8_t const>), void* ctx) const {
    if (impl_) impl_->for_each_entry_impl(cb, ctx);
}

// =============================================================================
// compact_db
// =============================================================================

compact_db::compact_db() = default;
compact_db::~compact_db() = default;
compact_db::compact_db(compact_db&&) noexcept = default;
compact_db& compact_db::operator=(compact_db&&) noexcept = default;

result<compact_db> compact_db::open(std::string_view path, bool remove_existing) {
    compact_db db;
    db.impl_ = std::make_unique<detail::database_impl>();
    auto r = db.impl_->configure(path, remove_existing, storage_mode::compact);
    if (!r) return std::unexpected(r.error());
    return db;
}

result<compact_db> compact_db::open_for_testing(std::string_view path, bool remove_existing) {
    compact_db db;
    db.impl_ = std::make_unique<detail::database_impl>();
    auto r = db.impl_->configure_for_testing(path, remove_existing, storage_mode::compact);
    if (!r) return std::unexpected(r.error());
    return db;
}

result<bool> compact_db::insert(raw_outpoint const& key, uint32_t file_number, uint32_t offset, uint32_t height) {
    if (!impl_) return std::unexpected(error_code::closed);
    return impl_->compact_insert_typed(key, height, file_number, offset);
}

result<compact_find_result> compact_db::find(raw_outpoint const& key, uint32_t height) const {
    if (!impl_) return std::unexpected(error_code::closed);
    auto r = impl_->compact_find_typed(key, height);
    if (!r) return std::unexpected(error_code::not_found);
    return std::move(*r);
}

std::pair<flat_map<raw_outpoint, compact_find_result>, std::vector<deferred_lookup_entry>>
compact_db::process_pending_lookups() {
    if (!impl_) return {};
    return impl_->compact_process_pending_lookups();
}

void compact_db::for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, uint32_t, uint32_t), void* ctx) const {
    if (impl_) impl_->compact_for_each_entry_typed(cb, ctx);
}

} // namespace utxoz
