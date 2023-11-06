#pragma once

#include <nlohmann/json.hpp>
#include "Layout.h"

namespace classgraph {
    class LayoutIO {
        std::optional<nlohmann::json> json{};
        std::optional<Layout> read_layout{};
    public:
        void read_json(std::istream &in);
        void read_json(const std::string& filename);

        void write_layout_to_canvas(const Layout& compatible, std::ostream& out) const;

        void write_new_layout(const Layout& compatible, std::ostream& out) const;
        void write_new_layout(const Layout& compatible, const std::string& filename) const;

        size_t term_count() const;

        const Layout& get_layout() const;
    };
}