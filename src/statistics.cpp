// Copyright (c) 2016-present Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file statistics.cpp
 * @brief Statistics implementation
 */

#include <utxoz/statistics.hpp>
#include <algorithm>
#include <numeric>

namespace utxoz {

void search_stats::add_record(uint32_t access_height, uint32_t insertion_height, 
                             uint32_t depth, bool cache_hit, bool found, char operation) {
    records_.emplace_back(search_record{
        access_height, insertion_height, depth, cache_hit, found, operation
    });
}

void search_stats::reset() {
    records_.clear();
}

search_summary search_stats::get_summary() const {
    search_summary summary;
    
    if (records_.empty()) {
        return summary;
    }
    
    summary.total_operations = records_.size();
    
    size_t total_depth = 0;
    size_t total_age = 0;
    size_t cache_accesses = 0;
    
    for (auto const& record : records_) {
        if (record.found) {
            ++summary.found_operations;
            total_age += record.get_utxo_age();
            
            if (record.depth == 0) {
                ++summary.current_version_hits;
            }
        }
        
        total_depth += record.depth;
        
        if (record.depth > 0) {
            ++cache_accesses;
            if (record.is_cache_hit) {
                ++summary.cache_hits;
            }
        }
    }
    
    // Calculate rates and averages
    if (summary.total_operations > 0) {
        summary.hit_rate = double(summary.found_operations) / summary.total_operations;
        summary.avg_depth = double(total_depth) / summary.total_operations;
    }
    
    if (summary.found_operations > 0) {
        summary.avg_utxo_age = double(total_age) / summary.found_operations;
    }
    
    if (cache_accesses > 0) {
        summary.cache_hit_rate = double(summary.cache_hits) / cache_accesses;
    }
    
    return summary;
}

} // namespace utxoz
