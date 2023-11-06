//
// Created by Tim Herchen on 11/5/23.
//

#include "classgraph/LayoutIO.h"
#include <fstream>
#include <iostream>
#include "safe_int_cast.h"

using namespace anematode;

void classgraph::LayoutIO::read_json(std::istream &in) {
    json = nlohmann::json::parse(in);

    const auto& j = json.value();
    const auto& terms_in = j["curriculum_terms"];

    assert(terms_in.is_array());

    decltype(read_layout->node_info) node_info;
    decltype(read_layout->terms) terms;

    size_t term_count = terms_in.size();
    assert(term_count <= MAX_TERMS);
    terms.resize(term_count);
    
    int term_i = 0;
    for (const auto& val : terms_in) {
        const auto& items = val["curriculum_items"];
        assert(items.is_array());
        
        int initial_class_order = 0;

        for (const auto& class_ : items) {
            assert(class_["id"].is_number_integer());
            auto class_id = class_["id"].get<int>();
            assert(class_id >= 0 && class_id <= MAX_CLASS_ID);
            
            Node classNode { checked_int_cast<int8_t>(term_i),
                             checked_int_cast<uint8_t>(initial_class_order),
                             checked_int_cast<uint8_t>(class_id) };

            auto& prereqs = classNode.prereqs;
            
            const auto& prereqs_in = class_["curriculum_requisites"];
            assert(prereqs_in.is_array());
            
            size_t prereq_count = prereqs_in.size();
            assert(prereq_count <= MAX_PREREQS);

            int prereq_i = 0;
            for (const auto& prereq_pair : prereqs_in) {
                const auto& source_id = prereq_pair["source_id"];
                const auto& target_id = prereq_pair["target_id"];

                assert(source_id.is_number_integer());
                assert(target_id.is_number_integer());

                auto source_id_number = source_id.get<int>();
                auto target_id_number = target_id.get<int>();

                assert(source_id_number >= 0 && source_id_number <= MAX_CLASS_ID);
                assert(target_id_number >= 0 && target_id_number <= MAX_CLASS_ID);
                assert(target_id_number == class_id);

                prereqs[prereq_i] = source_id_number;
                prereq_i += 1;
            }

            initial_class_order += 1;
        }
    }

    read_layout.emplace(node_info, std::move(terms));
}

void classgraph::LayoutIO::read_json(const std::string &filename) {
    std::cout << "Reading file " << filename << "\n";
    std::ifstream in { filename };
    assert(in.is_open());

    read_json(in);
}

void classgraph::LayoutIO::write_new_layout(const classgraph::Layout &compatible, const std::string& filename) const {
    std::ofstream out { filename };
    assert(out.is_open());

    write_new_layout(compatible, out);
}

void classgraph::LayoutIO::write_new_layout(const classgraph::Layout &compatible, std::ostream &out) const {
    const auto& my_layout = read_layout.value();
    assert(my_layout.is_compatible_with(compatible));

    const auto& my_json_terms = json.value()["curriculum_terms"];

    auto new_json = json.value();
    auto new_json_terms = new_json["curriculum_terms"];

    my_layout.for_each_term([&] (const auto& term, int term_i) {
        int i = 0;
        for (ClassID id : term) {
            // Find order of id in original
            auto original_order = my_layout.get_class(id).order;
            new_json_terms[i] = my_json_terms[original_order];

            i += 1;
        }
    });

   out << new_json;
}

const classgraph::Layout& classgraph::LayoutIO::get_layout() const {
    return read_layout.value();
}

size_t classgraph::LayoutIO::term_count() const {
    return read_layout.value().terms.size();
}
