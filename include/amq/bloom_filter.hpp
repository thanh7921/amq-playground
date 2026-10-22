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

class StandardBloomFilter
{
public:
    StandardBloomFilter(size_t expected_items, double target_fpr)
    {
        // Optimal bits: m = - (n * ln(p)) / (ln(2)^2)
        const double ln2_sq = 0.4804530139182014;
        const double m = -static_cast<double>(expected_items) * std::log(target_fpr) / ln2_sq;
        num_bits_ = std::max<size_t>(64, static_cast<size_t>(std::ceil(m)));

        // Optimal hash count: k = (m / n) * ln(2)
        const double k = (static_cast<double>(num_bits_) / static_cast<double>(expected_items))
                         * 0.6931471805599453;
        num_hashes_ = std::max<size_t>(1, static_cast<size_t>(std::round(k)));

        // 64-bit word storage
        const size_t num_words = (num_bits_ + 63) / 64;
        bits_.resize(num_words, 0ULL);
        num_bits_ = num_words * 64;  // align to 64 bits
    }

    void insert(uint64_t key) noexcept
    {
        const HashPair hashes = HashPair::create(key);
        for (size_t i = 0; i < num_hashes_; ++i)
        {
            const size_t bit_idx = hashes.nth_hash(i) % num_bits_;
            bits_[bit_idx / 64] |= (1ULL << (bit_idx % 64));
        }
    }

    [[nodiscard]] bool contains(uint64_t key) const noexcept
    {
        const HashPair hashes = HashPair::create(key);
        for (size_t i = 0; i < num_hashes_; ++i)
        {
            const size_t bit_idx = hashes.nth_hash(i) % num_bits_;
            if (!(bits_[bit_idx / 64] & (1ULL << (bit_idx % 64))))
            {
                return false;  // Early exit on first unset bit
            }
        }
        return true;
    }

    void clear() noexcept { std::fill(bits_.begin(), bits_.end(), 0ULL); }

    [[nodiscard]] size_t size_in_bytes() const noexcept { return bits_.size() * sizeof(uint64_t); }

    [[nodiscard]] size_t num_hashes() const noexcept { return num_hashes_; }

    [[nodiscard]] size_t num_bits() const noexcept { return num_bits_; }

private:
    size_t num_bits_;
    size_t num_hashes_;
    std::vector<uint64_t> bits_;
};

// Compile-time assertion that StandardBloomFilter satisfies the concept
static_assert(AMQFilter<StandardBloomFilter>);

}  // namespace amq
