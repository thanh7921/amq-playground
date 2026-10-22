#pragma once

#include "amq/concept.hpp"
#include "amq/hash.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace amq
{

class CountingBloomFilter
{
public:
    static constexpr uint8_t MAX_COUNT = 15;  // 4-bit counter saturation threshold

    CountingBloomFilter(size_t expected_items, double target_fpr)
    {
        // Optimal slots: m = - (n * ln(p)) / (ln(2)^2)
        const double ln2_sq = 0.4804530139182014;
        const double m = -static_cast<double>(expected_items) * std::log(target_fpr) / ln2_sq;
        num_slots_ = std::max<size_t>(64, static_cast<size_t>(std::ceil(m)));

        // Optimal hash count: k = (m / n) * ln(2)
        const double k = (static_cast<double>(num_slots_) / static_cast<double>(expected_items))
                         * 0.6931471805599453;
        num_hashes_ = std::max<size_t>(1, static_cast<size_t>(std::round(k)));

        // 4 bits per counter -> 2 counters per byte
        const size_t num_bytes = (num_slots_ + 1) / 2;
        storage_.resize(num_bytes, 0);
        num_slots_ = num_bytes * 2;  // Align to whole bytes
    }

    void insert(uint64_t key) noexcept
    {
        const HashPair hashes = HashPair::create(key);
        for (size_t i = 0; i < num_hashes_; ++i)
        {
            const size_t slot = hashes.nth_hash(i) % num_slots_;
            increment(slot);
        }
    }

    [[nodiscard]] bool contains(uint64_t key) const noexcept
    {
        const HashPair hashes = HashPair::create(key);
        for (size_t i = 0; i < num_hashes_; ++i)
        {
            const size_t slot = hashes.nth_hash(i) % num_slots_;
            if (get_counter(slot) == 0)
            {
                return false;  // Early exit
            }
        }
        return true;
    }

    // Removes an element. Returns false if the element was not found.
    bool remove(uint64_t key) noexcept
    {
        // Guard against removing items that are not in the filter (causes false negatives for other
        // items)
        if (!contains(key))
        {
            return false;
        }

        const HashPair hashes = HashPair::create(key);
        for (size_t i = 0; i < num_hashes_; ++i)
        {
            const size_t slot = hashes.nth_hash(i) % num_slots_;
            decrement(slot);
        }
        return true;
    }

    void clear() noexcept { std::fill(storage_.begin(), storage_.end(), static_cast<uint8_t>(0)); }

    [[nodiscard]] size_t size_in_bytes() const noexcept
    {
        return storage_.size() * sizeof(uint8_t);
    }

    [[nodiscard]] size_t num_hashes() const noexcept { return num_hashes_; }

    [[nodiscard]] size_t num_slots() const noexcept { return num_slots_; }

private:
    [[nodiscard]] inline uint8_t get_counter(size_t slot) const noexcept
    {
        const uint8_t byte = storage_[slot / 2];
        return (slot & 1) ? ((byte >> 4) & 0x0F) : (byte & 0x0F);
    }

    inline void increment(size_t slot) noexcept
    {
        const size_t byte_idx = slot / 2;
        uint8_t byte = storage_[byte_idx];
        const bool is_high_nibble = (slot & 1);

        uint8_t val = is_high_nibble ? ((byte >> 4) & 0x0F) : (byte & 0x0F);

        // Saturating counter: stay at MAX_COUNT to avoid wrap-around
        if (val < MAX_COUNT)
        {
            val++;
            if (is_high_nibble)
            {
                byte = static_cast<uint8_t>((byte & 0x0F) | (val << 4));
            }
            else
            {
                byte = static_cast<uint8_t>((byte & 0xF0) | val);
            }
            storage_[byte_idx] = byte;
        }
    }

    inline void decrement(size_t slot) noexcept
    {
        const size_t byte_idx = slot / 2;
        uint8_t byte = storage_[byte_idx];
        const bool is_high_nibble = (slot & 1);

        uint8_t val = is_high_nibble ? ((byte >> 4) & 0x0F) : (byte & 0x0F);

        // If saturated, remain saturated to preserve conservative guarantees
        if (val > 0 && val < MAX_COUNT)
        {
            val--;
            if (is_high_nibble)
            {
                byte = static_cast<uint8_t>((byte & 0x0F) | (val << 4));
            }
            else
            {
                byte = static_cast<uint8_t>((byte & 0xF0) | val);
            }
            storage_[byte_idx] = byte;
        }
    }

    size_t num_slots_;
    size_t num_hashes_;
    std::vector<uint8_t> storage_;
};

// Static verification of both concepts
static_assert(AMQFilter<CountingBloomFilter>);
static_assert(DeletableAMQFilter<CountingBloomFilter>);

}  // namespace amq
