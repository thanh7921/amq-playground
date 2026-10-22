#pragma once

#include <cstdint>

namespace amq
{

// Fast, high-quality 64-bit integer mixer (SplitMix64)
[[nodiscard]] inline constexpr uint64_t splitmix64(uint64_t x) noexcept
{
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// Kirsch-Mitzenmacher technique: generate k distinct hashes from two 64-bit values:
// g_i(x) = h1(x) + i * h2(x)
struct HashPair
{
    uint64_t h1;
    uint64_t h2;

    [[nodiscard]] static constexpr HashPair create(uint64_t key) noexcept
    {
        uint64_t h1 = splitmix64(key);
        // Salt the second hash to make it independent
        uint64_t h2 = splitmix64(key ^ 0x517cc1b727220a95ULL);
        // Ensure h2 is odd so it spans all bucket slots under modular arithmetic
        return {h1, h2 | 1ULL};
    }

    [[nodiscard]] constexpr uint64_t nth_hash(std::size_t i) const noexcept { return h1 + i * h2; }
};

}  // namespace amq
