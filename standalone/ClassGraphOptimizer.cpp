
#include "classgraph/Layout.h"
#include "classgraph/LayoutIO.h"
#include <iostream>

#include <cxxopts.hpp>

int main(int argc, char** argv) {
    using namespace classgraph;

    cxxopts::Options options { "ClassGraphOptimizer", "Optimize ordering of class data" };
    options.add_options()
            ("in_file", "Input file", cxxopts::value<std::string>())
            ("out_file", "Output path (default: out.json)", cxxopts::value<std::string>()->default_value("./out.json"));

    options.parse_positional({ "in_file", "out_file" });

    auto result = options.parse(argc, argv);
    auto in = result["in_file"].as<std::string>();
    auto out = result["out_file"].as<std::string>();

    LayoutIO io;
    io.read_json(in);
}