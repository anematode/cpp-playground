//
// Created by Tim Herchen on 11/5/23.
//

#pragma once

#include <istream>
#include <array>
#include <vector>
#include <cassert>

namespace classgraph {
    constexpr int MAX_PREREQS = 8;
    constexpr int MAX_TERMS = 12;
    constexpr int MAX_CLASS_ID = 254;
    constexpr int NO_CLASS_ID = 255;

    using ClassID = uint8_t;
    using Prereqs = std::array<ClassID, MAX_PREREQS>;

    struct FPoint {
        float x;
        float y;
    };

    struct Point {
        int x;
        int y;

        Point operator-(const Point& other) {
            return { .x = x - other.x, .y = y - other.y };
        }
    };

    struct SmallPoint {
        uint8_t x;
        uint8_t y;

        [[nodiscard]] uint16_t asU16() const {
            return *reinterpret_cast<const uint16_t*>(this);
        }

        Point point() const {
            return { .x = x, .y = y };
        }
    };


    struct Connexion {
        SmallPoint pt1;
        SmallPoint pt2;
    };

    struct Intersection {
        Connexion c1;
        Connexion c2;
    };

    struct Node {
        int8_t term;
        uint8_t order{};
        uint8_t class_id{};

        Prereqs prereqs{};

        Node();
        Node(int8_t term, uint8_t order, uint8_t class_id);

        int prereq_count() const;

        template <typename Lambda>
        inline void for_each_prereq(Lambda callback) const {
            for (auto prereq : prereqs) {
                if (prereq == NO_CLASS_ID) {
                    break;
                }

                callback(prereq);
            }
        }

        SmallPoint small_point() const {
            return { .x = static_cast<uint8_t>(term), .y = order };
        }
    };

    using NodeInfo = std::array<Node, MAX_CLASS_ID>;
    using Terms = std::vector<std::vector<uint8_t>>;

    class Layout {
        NodeInfo node_info{};
        Terms terms{};

        std::vector<Connexion> resolved_connexions{};
        std::vector<Intersection> possible_intersections{};

    public:
        Layout() = delete;
        Layout(const NodeInfo& info, Terms&& terms);

        template <typename Lambda>
        void for_each_class(Lambda callback) const {
            for (const auto& node : node_info) {
                if (node.class_id != NO_CLASS_ID) {
                    callback(node);
                }
            }
        }

        template <typename Lambda>
        void for_each_connexion(Lambda callback) const {

        }

        template <typename Lambda>
        void for_each_term(Lambda callback) const {
            int i = 0;
            for (const auto& term : terms) {
                callback(term, i);
            }
        }

        void swap_nodes(Node& a, Node& b);

        const Node& get_class(ClassID classID) const {
            const auto& node = node_info.at(classID);
            assert(node.class_id != NO_CLASS_ID);
            return node;
        }

        Node& get_class_mut(ClassID classID) {
            auto& node = node_info.at(classID);
            assert(node.class_id != NO_CLASS_ID);
            return node;
        }

        static Layout read(std::istream& in);

        void shuffle();

        bool is_compatible_with(const Layout& other) const;

        void compute_connexions();
        void compute_possible_intersections();

        friend class LayoutIO;
    };
}
