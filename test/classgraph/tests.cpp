#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/benchmark/catch_benchmark_all.hpp>
#include "classgraph/Layout.h"
#include "classgraph/Swaps.h"

#include <fstream>

using namespace classgraph;

TEST_CASE("Layout read/write") {

}

TEST_CASE("Swaps") {
    using namespace Catch::Matchers;

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

    std::unordered_map<int, std::vector<uint16_t>> cow;

    auto create = [&] (int size) {
        std::vector<uint16_t> c;
        c.resize(size);
        for (int i = 0; i < size; ++i) { \
            c[i] = i % 5;
        }
        cow[size] = c;
    };

#define CREATE_BENCHMARK(size_, size_label, native, native_label) \
    BENCHMARK("swap_small_points_vector, size " size_label ", native " native_label) { \
      auto& c = cow[size_]; \
      for (int j = 0; j < 10; ++j) { \
          swap_small_points_vector<native>(c, 2, 3); \
      } \
      return c; \
  };
#define CREATE_2BENCHMARKS(size_, size_label) \
    create(size_); \
    CREATE_BENCHMARK(size_, size_label, false, "no") \
    CREATE_BENCHMARK(size_, size_label, true, "yes")

    CREATE_2BENCHMARKS(8, "8")
    CREATE_2BENCHMARKS(16, "16")
    CREATE_2BENCHMARKS(64, "64")
    CREATE_2BENCHMARKS(256, "256")
    CREATE_2BENCHMARKS(1024, "1024")
    CREATE_2BENCHMARKS(2048, "2048")
    CREATE_2BENCHMARKS(4096, "4096")
}
