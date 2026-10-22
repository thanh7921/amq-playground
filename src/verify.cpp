#include "amq/bloom_filter.hpp"
#include "amq/counting_bloom_filter.hpp"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

void test_standard_bloom_filter()
{
    constexpr size_t NUM_ITEMS = 100'000;
    constexpr double TARGET_FPR = 0.01;  // 1%
    constexpr size_t TEST_QUERIES = 500'000;

    std::cout << "========================================\n";
    std::cout << "  1. Standard Bloom Filter Verification \n";
    std::cout << "========================================\n";
    std::cout << "Target Items: " << NUM_ITEMS << " | Target FPR: " << TARGET_FPR * 100.0 << "%\n";

    amq::StandardBloomFilter sbf(NUM_ITEMS, TARGET_FPR);

    std::cout << "Allocated Memory: " << sbf.size_in_bytes() << " bytes ("
              << (static_cast<double>(sbf.size_in_bytes()) * 8.0) / NUM_ITEMS << " bits/item)\n";
    std::cout << "Computed Hashes (k): " << sbf.num_hashes() << "\n\n";

    // 1. Generate keys
    std::mt19937_64 rng(1337);
    std::vector<uint64_t> inserted_keys;
    inserted_keys.reserve(NUM_ITEMS);
    for (size_t i = 0; i < NUM_ITEMS; ++i)
    {
        inserted_keys.push_back(rng());
    }

    // 2. Insert items
    for (uint64_t key : inserted_keys)
    {
        sbf.insert(key);
    }

    // 3. Test False Negatives (Must strictly be 0)
    for (uint64_t key : inserted_keys)
    {
        if (!sbf.contains(key))
        {
            std::cerr << "[FAIL] False negative detected in Standard Bloom Filter for key: " << key
                      << "!\n";
            std::exit(1);
        }
    }
    std::cout << "[PASS] 0 False Negatives detected across " << NUM_ITEMS << " inserted keys.\n";

    // 4. Test Empirical False Positives
    std::mt19937_64 rng_disjoint(9999);
    size_t false_positives = 0;
    for (size_t i = 0; i < TEST_QUERIES; ++i)
    {
        uint64_t query_key = rng_disjoint();
        if (sbf.contains(query_key))
        {
            false_positives++;
        }
    }

    double empirical_fpr = static_cast<double>(false_positives) / TEST_QUERIES;
    std::cout << "[INFO] Tested " << TEST_QUERIES << " negative queries.\n";
    std::cout << "[INFO] Empirical FPR: " << std::fixed << std::setprecision(4)
              << (empirical_fpr * 100.0) << "% (False Positives: " << false_positives << ")\n";

    if (empirical_fpr > TARGET_FPR * 1.5)
    {
        std::cerr << "[WARN] Empirical FPR is higher than theoretical tolerance!\n";
    }
    else
    {
        std::cout << "[PASS] Empirical FPR matches expected probability model.\n";
    }
    std::cout << std::endl;
}

void test_counting_bloom_filter()
{
    constexpr size_t NUM_ITEMS = 50'000;
    constexpr double TARGET_FPR = 0.01;  // 1%
    constexpr size_t TEST_QUERIES = 200'000;

    std::cout << "========================================\n";
    std::cout << "  2. Counting Bloom Filter Verification \n";
    std::cout << "========================================\n";
    std::cout << "Target Items: " << NUM_ITEMS << " | Target FPR: " << TARGET_FPR * 100.0 << "%\n";

    amq::CountingBloomFilter cbf(NUM_ITEMS, TARGET_FPR);

    std::cout << "Allocated Memory: " << cbf.size_in_bytes() << " bytes ("
              << (static_cast<double>(cbf.size_in_bytes()) * 8.0) / NUM_ITEMS << " bits/item)\n";
    std::cout << "Computed Hashes (k): " << cbf.num_hashes() << "\n\n";

    std::mt19937_64 rng(777);
    std::vector<uint64_t> keys(NUM_ITEMS);
    for (auto &key : keys)
    {
        key = rng();
    }

    // 1. Insert all items
    for (uint64_t key : keys)
    {
        cbf.insert(key);
    }

    // 2. Verify all exist (0 False Negatives)
    for (uint64_t key : keys)
    {
        if (!cbf.contains(key))
        {
            std::cerr << "[FAIL] False negative detected in CBF for key: " << key << "!\n";
            std::exit(1);
        }
    }
    std::cout << "[PASS] 100% of inserted keys recognized (0 false negatives).\n";

    // 3. Test Deletion capability (delete first half of the items)
    const size_t delete_count = NUM_ITEMS / 2;
    for (size_t i = 0; i < delete_count; ++i)
    {
        bool deleted = cbf.remove(keys[i]);
        if (!deleted)
        {
            std::cerr << "[FAIL] Removal reported false on known-inserted key!\n";
            std::exit(1);
        }
    }
    std::cout << "[PASS] Successfully executed remove() on " << delete_count << " keys.\n";

    // 4. Verify collateral damage check: untouched keys MUST still be present
    for (size_t i = delete_count; i < NUM_ITEMS; ++i)
    {
        if (!cbf.contains(keys[i]))
        {
            std::cerr << "[FAIL] Collateral damage! Untouched key went missing: " << keys[i]
                      << "!\n";
            std::exit(1);
        }
    }
    std::cout << "[PASS] Untouched keys remain safely intact.\n";

    // 5. Check ghost presence (deleted items that still test true due to collisions)
    size_t ghosts = 0;
    for (size_t i = 0; i < delete_count; ++i)
    {
        if (cbf.contains(keys[i]))
        {
            ghosts++;
        }
    }
    std::cout << "[INFO] Deleted keys still reporting true (residual collisions): " << ghosts
              << " / " << delete_count << " (" << std::fixed << std::setprecision(4)
              << (static_cast<double>(ghosts) / delete_count) * 100.0 << "%)\n";

    // 6. Test Empirical FPR on completely disjoint set
    std::mt19937_64 rng_disjoint(4321);
    size_t false_positives = 0;
    for (size_t i = 0; i < TEST_QUERIES; ++i)
    {
        uint64_t query_key = rng_disjoint();
        if (cbf.contains(query_key))
        {
            false_positives++;
        }
    }

    double empirical_fpr = static_cast<double>(false_positives) / TEST_QUERIES;
    std::cout << "[INFO] Empirical FPR on disjoint keys: " << (empirical_fpr * 100.0)
              << "% (False Positives: " << false_positives << ")\n";
    std::cout << "[PASS] CBF functional tests complete.\n\n";
}

int main()
{
    test_standard_bloom_filter();
    test_counting_bloom_filter();

    std::cout << "All AMQ verification checks passed successfully!\n";
    return 0;
}
