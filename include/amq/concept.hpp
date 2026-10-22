#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace amq
{

template <typename T>
concept AMQFilter = requires(T filter, const T const_filter, uint64_t key) {
    { filter.insert(key) } -> std::same_as<void>;
    { const_filter.contains(key) } -> std::same_as<bool>;
    { const_filter.size_in_bytes() } -> std::convertible_to<std::size_t>;
    { filter.clear() } -> std::same_as<void>;
};

// Extends AMQFilter with safe deletion capabilities
template <typename T>
concept DeletableAMQFilter = AMQFilter<T> && requires(T filter, uint64_t key) {
    // Returns false if key is definitely not present, true if decrement succeeded
    { filter.remove(key) } -> std::same_as<bool>;
};

}  // namespace amq
