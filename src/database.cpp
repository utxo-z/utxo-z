// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file database.cpp
 * @brief Database interface implementation — db_base, full_db, reference_db
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

deletion_progress db_base::apply_deletes(std::span<deferred_deletion_entry const> requests) {
    // No result<> to carry these, so they arrive the same way any other
    // incomplete batch does: everything still owed, and why.
    //
    // Through the same deduplication the applying path uses. Copying the span
    // verbatim made a refusal the one place where a repeated outpoint came back
    // more than once, so the contract on deletion_progress held everywhere
    // except where a caller is most likely to be looping over the result.
    if ( ! impl_) return detail::refuse_deletions(requests, error_code::closed);
    if ( ! impl_->refuse_if_recovery_pending()) {
        return detail::refuse_deletions(requests, error_code::recovery_required);
    }
    if ( ! impl_->refuse_if_inspection_only()) {
        return detail::refuse_deletions(requests, error_code::inspection_only);
    }
    return impl_->apply_deletes(requests);
}

durability_level platform_durability() noexcept {
    switch (detail::platform_sync_support()) {
        case detail::sync_support::full:      return durability_level::full;
        case detail::sync_support::file_only: return durability_level::contents_only;
        case detail::sync_support::none:      return durability_level::none;
    }
    return durability_level::none;
}

result<> db_base::sync() {
    if ( ! impl_) return std::unexpected(error_code::closed);
    // No recovery guard here — sync() is what a caller reaches for *because*
    // something went wrong — but an inspection has nothing to make durable that
    // it did not find already durable.
    if (auto const usable = impl_->refuse_if_inspection_only(); ! usable) {
        return std::unexpected(usable.error());
    }
    return impl_->sync();
}

result<> db_base::compact_all() {
    if (!impl_) return std::unexpected(error_code::closed);
    if (auto const ready = impl_->refuse_if_recovery_pending(); ! ready) return std::unexpected(ready.error());
    if (auto const usable = impl_->refuse_if_inspection_only(); ! usable) return std::unexpected(usable.error());
    return impl_->compact_all();
}

result<> db_base::for_each_key_impl(void(*cb)(void*, raw_outpoint const&), void* ctx) const {
    if ( ! impl_) return std::unexpected(error_code::closed);
    if (auto const ready = impl_->refuse_if_recovery_pending(); ! ready) return std::unexpected(ready.error());
    if (auto const usable = impl_->refuse_if_inspection_only(); ! usable) return std::unexpected(usable.error());
    return impl_->for_each_key_impl(cb, ctx);
}

database_statistics db_base::get_statistics() {
    if (!impl_) return {};
    return impl_->get_statistics();
}

void db_base::print_statistics() {
    if (impl_) impl_->print_statistics();
}

result<census_report> db_base::census(census_options const& options) const {
    if ( ! impl_) return std::unexpected(error_code::closed);
    return impl_->census(options);
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

result<full_db> full_db::open(std::filesystem::path path, bool remove_existing) {
    full_db db;
    db.impl_ = std::make_unique<detail::database_impl>();
    auto r = db.impl_->configure(std::move(path), remove_existing, storage_mode::full);
    if (!r) return std::unexpected(r.error());
    return db;
}

result<full_db> full_db::open_for_testing(std::filesystem::path path, bool remove_existing) {
    full_db db;
    db.impl_ = std::make_unique<detail::database_impl>();
    auto r = db.impl_->configure_for_testing(std::move(path), remove_existing, storage_mode::full);
    if (!r) return std::unexpected(r.error());
    return db;
}

result<full_db> full_db::open_for_inspection(std::filesystem::path path) {
    full_db db;
    db.impl_ = std::make_unique<detail::database_impl>();
    auto r = db.impl_->open_for_inspection(std::move(path), storage_mode::full);
    if ( ! r) return std::unexpected(r.error());
    return db;
}

result<full_db> full_db::open_for_inspection_for_testing(std::filesystem::path path) {
    full_db db;
    db.impl_ = std::make_unique<detail::database_impl>();
    auto r = db.impl_->open_for_inspection_for_testing(std::move(path), storage_mode::full);
    if ( ! r) return std::unexpected(r.error());
    return db;
}

result<bool> full_db::insert(raw_outpoint const& key, output_data_span value, uint32_t height) {
    if (!impl_) return std::unexpected(error_code::closed);
    if (auto const ready = impl_->refuse_if_recovery_pending(); ! ready) return std::unexpected(ready.error());
    if (auto const usable = impl_->refuse_if_inspection_only(); ! usable) return std::unexpected(usable.error());
    return impl_->insert(key, value, height);
}

result<full_find_result> full_db::find(raw_outpoint const& key, uint32_t height) const {
    if (!impl_) return std::unexpected(error_code::closed);
    if (auto const ready = impl_->refuse_if_recovery_pending(); ! ready) return std::unexpected(ready.error());
    if (auto const usable = impl_->refuse_if_inspection_only(); ! usable) return std::unexpected(usable.error());
    auto r = impl_->full_find(key, height);
    // not_resolved, not not_found: the active versions did not have it, and they
    // are all this looked at. Absence is resolve()'s to establish.
    if (!r) return std::unexpected(error_code::not_resolved);
    return std::move(*r);
}

result<full_resolution> full_db::resolve(std::span<lookup_request const> requests) const {
    if ( ! impl_) return std::unexpected(error_code::closed);
    if (auto const ready = impl_->refuse_if_recovery_pending(); ! ready) return std::unexpected(ready.error());
    if (auto const usable = impl_->refuse_if_inspection_only(); ! usable) return std::unexpected(usable.error());
    return impl_->full_resolve(requests);
}

result<> full_db::for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, std::span<uint8_t const>), void* ctx) const {
    if ( ! impl_) return std::unexpected(error_code::closed);
    if (auto const ready = impl_->refuse_if_recovery_pending(); ! ready) return std::unexpected(ready.error());
    if (auto const usable = impl_->refuse_if_inspection_only(); ! usable) return std::unexpected(usable.error());
    return impl_->for_each_entry_impl(cb, ctx);
}

// =============================================================================
// reference_db
// =============================================================================

reference_db::reference_db() = default;
reference_db::~reference_db() = default;
reference_db::reference_db(reference_db&&) noexcept = default;
reference_db& reference_db::operator=(reference_db&&) noexcept = default;

result<reference_db> reference_db::open(std::filesystem::path path, bool remove_existing) {
    reference_db db;
    db.impl_ = std::make_unique<detail::database_impl>();
    auto r = db.impl_->configure(std::move(path), remove_existing, storage_mode::reference);
    if (!r) return std::unexpected(r.error());
    return db;
}

result<reference_db> reference_db::open_for_testing(std::filesystem::path path, bool remove_existing) {
    reference_db db;
    db.impl_ = std::make_unique<detail::database_impl>();
    auto r = db.impl_->configure_for_testing(std::move(path), remove_existing, storage_mode::reference);
    if (!r) return std::unexpected(r.error());
    return db;
}

result<reference_db> reference_db::open_for_inspection(std::filesystem::path path) {
    reference_db db;
    db.impl_ = std::make_unique<detail::database_impl>();
    auto r = db.impl_->open_for_inspection(std::move(path), storage_mode::reference);
    if ( ! r) return std::unexpected(r.error());
    return db;
}

result<reference_db> reference_db::open_for_inspection_for_testing(std::filesystem::path path) {
    reference_db db;
    db.impl_ = std::make_unique<detail::database_impl>();
    auto r = db.impl_->open_for_inspection_for_testing(std::move(path), storage_mode::reference);
    if ( ! r) return std::unexpected(r.error());
    return db;
}

result<bool> reference_db::insert(raw_outpoint const& key, uint32_t file_number, uint32_t offset, uint32_t height) {
    if (!impl_) return std::unexpected(error_code::closed);
    if (auto const ready = impl_->refuse_if_recovery_pending(); ! ready) return std::unexpected(ready.error());
    if (auto const usable = impl_->refuse_if_inspection_only(); ! usable) return std::unexpected(usable.error());
    return impl_->reference_insert_typed(key, height, file_number, offset);
}

result<reference_find_result> reference_db::find(raw_outpoint const& key, uint32_t height) const {
    if (!impl_) return std::unexpected(error_code::closed);
    if (auto const ready = impl_->refuse_if_recovery_pending(); ! ready) return std::unexpected(ready.error());
    if (auto const usable = impl_->refuse_if_inspection_only(); ! usable) return std::unexpected(usable.error());
    auto r = impl_->reference_find_typed(key, height);
    // not_resolved, not not_found: see full_db::find().
    if (!r) return std::unexpected(error_code::not_resolved);
    return std::move(*r);
}

result<reference_resolution> reference_db::resolve(std::span<lookup_request const> requests) const {
    if ( ! impl_) return std::unexpected(error_code::closed);
    if (auto const ready = impl_->refuse_if_recovery_pending(); ! ready) return std::unexpected(ready.error());
    if (auto const usable = impl_->refuse_if_inspection_only(); ! usable) return std::unexpected(usable.error());
    return impl_->reference_resolve(requests);
}

result<> reference_db::for_each_entry_impl(void(*cb)(void*, raw_outpoint const&, uint32_t, uint32_t, uint32_t), void* ctx) const {
    if ( ! impl_) return std::unexpected(error_code::closed);
    if (auto const ready = impl_->refuse_if_recovery_pending(); ! ready) return std::unexpected(ready.error());
    if (auto const usable = impl_->refuse_if_inspection_only(); ! usable) return std::unexpected(usable.error());
    return impl_->reference_for_each_entry_typed(cb, ctx);
}

} // namespace utxoz
