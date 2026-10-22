#include "amq/bloom_filter.hpp"
#include "amq/counting_bloom_filter.hpp"
#include <benchmark/benchmark.h>

#include <random>
#include <vector>

struct AMQFixture : public benchmark::Fixture
{
    static constexpr size_t NUM_ITEMS = 50'000;
    std::vector<uint64_t> keys;
    std::vector<uint64_t> absent_keys;

    std::unique_ptr<amq::StandardBloomFilter> sbf;
    std::unique_ptr<amq::CountingBloomFilter> cbf;

    void SetUp(const ::benchmark::State &) override
    {
        keys.reserve(NUM_ITEMS);
        absent_keys.reserve(NUM_ITEMS);

        std::mt19937_64 rng(42);
        std::mt19937_64 rng_absent(99);

        for (size_t i = 0; i < NUM_ITEMS; ++i)
        {
            keys.push_back(rng());
            absent_keys.push_back(rng_absent());
        }

        sbf = std::make_unique<amq::StandardBloomFilter>(NUM_ITEMS, 0.01);
        cbf = std::make_unique<amq::CountingBloomFilter>(NUM_ITEMS, 0.01);

        for (uint64_t k : keys)
        {
            sbf->insert(k);
            cbf->insert(k);
        }
    }
};

// --- Standard Bloom Filter ---
BENCHMARK_F(AMQFixture, BM_Standard_LookupHit)(benchmark::State &state)
{
    size_t i = 0;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(sbf->contains(keys[i % NUM_ITEMS]));
        ++i;
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(AMQFixture, BM_Standard_LookupMiss)(benchmark::State &state)
{
    size_t i = 0;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(sbf->contains(absent_keys[i % NUM_ITEMS]));
        ++i;
    }
    state.SetItemsProcessed(state.iterations());
}

// --- Counting Bloom Filter ---
BENCHMARK_F(AMQFixture, BM_Counting_LookupHit)(benchmark::State &state)
{
    size_t i = 0;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(cbf->contains(keys[i % NUM_ITEMS]));
        ++i;
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(AMQFixture, BM_Counting_LookupMiss)(benchmark::State &state)
{
    size_t i = 0;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(cbf->contains(absent_keys[i % NUM_ITEMS]));
        ++i;
    }
    state.SetItemsProcessed(state.iterations());
}

// Insertion & Deletion performance for CBF
BENCHMARK_F(AMQFixture, BM_Counting_Insert)(benchmark::State &state)
{
    amq::CountingBloomFilter local_cbf(NUM_ITEMS, 0.01);
    size_t i = 0;
    for (auto _ : state)
    {
        local_cbf.insert(keys[i % NUM_ITEMS]);
        ++i;
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(AMQFixture, BM_Counting_Remove)(benchmark::State &state)
{
    size_t i = 0;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(cbf->remove(keys[i % NUM_ITEMS]));
        ++i;
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_MAIN();
