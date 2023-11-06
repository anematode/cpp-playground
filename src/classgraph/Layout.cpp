//
// Created by Tim Herchen on 11/5/23.
//

#include "classgraph/Layout.h"
#include "safe_int_cast.h"
#include "classgraph/Swaps.h"
#include <cassert>
#include <algorithm>
#include <vector>
#include <utility>
#include <numeric>
#include <random>

using namespace anematode;

std::random_device rd;
std::mt19937 g(rd());

namespace classgraph {
    Layout::Layout(const NodeInfo& info, Terms&& terms) : node_info(info), terms(terms) {

    }

    Layout Layout::read(std::istream &in) {
        int term_count;
        assert(in >> term_count);
        assert(term_count > 0);

        NodeInfo nodes;
        Terms terms;

        std::fill(nodes.begin(), nodes.end(), Node(-1, 0, NO_CLASS_ID));

        terms.resize(term_count);
        for (int i = 0; i < term_count; ++i) {
            int term_i, count;

            assert(in >> term_i >> count);
            assert(term_i == i);

            for (int j = 0; j < count; ++j) {
                int class_id, prereq_count;

                assert(in >> class_id >> prereq_count);

                terms.at(term_i).push_back(class_id);
                nodes.at(class_id) = Node {
                    checked_int_cast<int8_t>(term_i),
                    checked_int_cast<uint8_t>(j),
                    checked_int_cast<uint8_t>(class_id)
                };

                // Read prereqs
                for (int k = 0; k < prereq_count; ++k) {
                    int prereq;

                    assert(in >> prereq);
                    assert(prereq != class_id);

                    nodes.at(class_id).prereqs[k] = prereq;
                }
            }
        }

        auto layout = Layout{ nodes, std::move(terms) };
        layout.compute_possible_intersections();
        return layout;
    }

    void Layout::compute_connexions() {
        resolved_connexions.clear();
        for_each_connexion([&] (const Connexion& c) {
            resolved_connexions.push_back(c);
        });
    }

    void Layout::swap_nodes(Node& a, Node& b) {
        assert(a.term == b.term);

        SmallPoint ap = a.small_point(), bp = b.small_point();
        std::swap(a.order, b.order);

        swap_small_points_vector(possible_intersections, ap.asU16(), bp.asU16());
    }

    void Layout::compute_possible_intersections() {

    }

    void Layout::shuffle() {
        for (auto & term : terms) {
            std::vector<int> orders;
            orders.resize(term.size());

            std::iota(orders.begin(), orders.end(), 0);
            std::shuffle(orders.begin(), orders.end(), g);

            for (int j = 0; j < term.size(); ++j) {
                // Fix orders in node_info
                node_info.at(term[j]).order = orders[j];
            }
        }
    }


    bool Layout::is_compatible_with(const Layout &other) const {
        if (terms.size() != other.terms.size()) {
            return false;
        }

        for (size_t i = 0; i < terms.size(); ++i) {
            auto my_term = terms[i];
            auto other_term = other.terms[i];

            if (my_term.size() != other_term.size()) {
                return false;
            }

            std::sort(my_term.begin(), my_term.end());
            std::sort(other_term.begin(), other_term.end());

            for (size_t j = 0; j < my_term.size(); ++j) {
                if (my_term[j] != other_term[j]) {
                    return false;
                }
            }
        }

        return true;
    }

    Node::Node() {
        term = -1;
    }

    Node::Node(int8_t term, uint8_t order, uint8_t class_id) : term(term), order(order), class_id(class_id) {
        std::fill(prereqs.begin(), prereqs.end(), NO_CLASS_ID);
    }

    int Node::prereq_count() const {
        int i = 0;
        for_each_prereq([&] (ClassID _) { ++i; });
        return i;
    }
}
