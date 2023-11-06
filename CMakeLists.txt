cmake_minimum_required(VERSION 3.0)
project(cpp_playground)

set(CMAKE_CXX_STANDARD 20)

Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.0.1
)

FetchContent_Declare(
        nhlomann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.2.0
)

FetchContent_Declare(
        cxxopts
        GIT_REPOSITORY https://github.com/jarro2783/cxxopts
        GIT_TAG        v3.0.0
)

FetchContent_MakeAvailable(Catch2)
FetchContent_MakeAvailable(nhlomann_json)
FetchContent_MakeAvailable(cxxopts)

include_directories(include)

set(classgraph_sources src/classgraph/Layout.cpp
        include/classgraph/Layout.h
        include/safe_int_cast.h
        include/classgraph/LayoutIO.h
        src/classgraph/LayoutIO.cpp
        include/classgraph/Swaps.h
)

add_executable(classgraph_optimizer ${classgraph_sources} standalone/ClassGraphOptimizer.cpp)
target_link_libraries(classgraph_optimizer PRIVATE nlohmann_json::nlohmann_json cxxopts::cxxopts)

add_executable(classgraph_tests ${classgraph_sources} test/classgraph/tests.cpp)
target_link_libraries(classgraph_tests PRIVATE Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
target_compile_options(classgraph_tests PRIVATE -march=native -O3)