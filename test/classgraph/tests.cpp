#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/benchmark/catch_benchmark_all.hpp>
#include "classgraph/Layout.h"
#include "classgraph/Swaps.h"

#include <fstream>

using namespace classgraph;

TEST_CASE("Layout read/write") {

}

TEST_CASE("Swap benchmarks") {
    std::unordered_map<int, std::vector<uint16_t>> cow_u16;
    std::unordered_map<int, std::vector<uint8_t>> cow_u8;

    auto create = [&]<typename T>(int size) -> std::vector<T>& {
        std::unordered_map<int, std::vector<T>>* m;
        if constexpr (std::is_same_v<T, uint16_t>) {
            m = &cow_u16;
        } else {
            m = &cow_u8;
        }
        if (m->contains(size)) {
            return (*m)[size];
        }

        std::vector<T> c;
        c.resize(size);
        for (int i = 0; i < size; ++i) { \
            c[i] = i % 5;
        }

        (*m)[size] = c;
        return (*m)[size];
    };
    auto create_u8 = [&] (int size) -> std::vector<uint8_t>& {
        return create.operator()<uint8_t>(size);
    };
    auto create_u16 = [&] (int size) -> std::vector<uint16_t>& {
        return create.operator()<uint16_t>(size);
    };

#define CREATE_BENCHMARK(size_, size_label, native, native_label) \
    BENCHMARK("swap_small_points_vector, size " size_label ", native " native_label) { \
      auto& c = create_u16(size_); \
      for (int j = 0; j < 10; ++j) { \
          swap_small_points_vector<native>(c, 2, 3); \
      } \
      return c; \
  };
#define CREATE_2BENCHMARKS(size_, size_label) \
    CREATE_BENCHMARK(size_, size_label, false, "no") \
    CREATE_BENCHMARK(size_, size_label, true, "yes")

    CREATE_2BENCHMARKS(8, "8")
    CREATE_2BENCHMARKS(16, "16")
    CREATE_2BENCHMARKS(64, "64")
    CREATE_2BENCHMARKS(256, "256")
    CREATE_2BENCHMARKS(1024, "1024")
    CREATE_2BENCHMARKS(2048, "2048")
    CREATE_2BENCHMARKS(4096, "4096")

#undef CREATE_BENCHMARK
#undef CREATE_2BENCHMARKS

#define CREATE_BENCHMARK(size_, size_label, native, native_label) \
    BENCHMARK("count_intersections, size " size_label ", native " native_label) { \
      auto& c = create_u8(size_); \
      return count_intersections<native>(c); \
  };
#define CREATE_2BENCHMARKS(size_, size_label) \
    CREATE_BENCHMARK(size_, size_label, false, "no") \
    CREATE_BENCHMARK(size_, size_label, true, "yes")

    CREATE_2BENCHMARKS(8, "8")
    CREATE_2BENCHMARKS(16, "16")
    CREATE_2BENCHMARKS(64, "64")
    CREATE_2BENCHMARKS(256, "256")
    CREATE_2BENCHMARKS(1024, "1024")
    CREATE_2BENCHMARKS(2048, "2048")
    CREATE_2BENCHMARKS(4096, "4096")
    CREATE_2BENCHMARKS(8192, "8192")
    CREATE_2BENCHMARKS(16384, "16384")

}

TEST_CASE("Swaps") {
    using namespace Catch::Matchers;


    // X shape
    std::vector<uint8_t> m1 = {
        0, 0, 1, 1, 0, 1, 1, 0,
    };
    REQUIRE(count_intersections<false>(m1) == IntersectionCounters { 1, 1 });
    // Two parallel
    std::vector<uint8_t> m2 = {
        0, 0, 1, 0, 0, 1, 1, 1
    };
    REQUIRE(count_intersections<false>(m2) == IntersectionCounters { 0, 0 });
    // Two meeting at one point
    std::vector<uint8_t> m3 = {
        0, 0, 1, 0, 0, 1, 1, 0
    };
    REQUIRE(count_intersections<false>(m3) == IntersectionCounters { 0, 1 });
    // Collinear
    REQUIRE(count_intersections<false>(std::vector<uint8_t>{ 0, 5, 3, 5, 1, 5, 2, 5 }) == IntersectionCounters { 0, 1 });

    uint64_t rng_state = 0;
    auto rng = [&] () {
        rng_state = rng_state * 3202 + 1010101;

        return rng_state;
    };

    for (int i = 0; i < 1000; ++i) {
        std::vector<uint8_t> m;
        m.resize(1000);
        
        for (int j = 0; j < 1000; ++j) {
            m[j] = rng() % 100;
        }

        REQUIRE(count_intersections<false>(m) == count_intersections<true>(m));
    }

    std::vector<uint16_t> k = {
        0, 5, 20, 10, 20, 5, 10, 101, 20000, 502, 10, 201, 10, 20, 2011, 102, 5
    };

    auto k2 = k;

    swap_small_points_vector<false>(k2, 20, 5);

    REQUIRE_THAT(k2, Equals(std::vector<uint16_t>{
            0, 20, 5, 10, 5, 20, 10, 101, 20000, 502, 10, 201, 10, 5, 2011, 102, 20
    }));

    swap_small_points_vector<true>(k, 20, 5);
    REQUIRE_THAT(k, Equals(k2));

    std::vector<uint16_t> k3, k4;
    for (int i = 0; i < 409; ++i) {
        k3.resize(i);

        for (int j = 0; j < k3.size(); ++j) {
            k3[j] = j % 5;
        }

        k4 = k3;

        swap_small_points_vector<false>(k3, 1, 2);
        swap_small_points_vector<true>(k4, 1, 2);

        REQUIRE_THAT(k3, Equals(k4));
    }

}

