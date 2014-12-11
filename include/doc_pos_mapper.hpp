#pragma once

#include <sdsl/int_vector.hpp>
#include <sdsl/rank_support_v5.hpp>
#include <string>
#include "doc_pos_mapper.hpp"

struct doc_pos_mapper {
    typedef typename sdsl::int_vector<>::size_type size_type;
    sdsl::bit_vector m_doc_border;
    sdsl::rank_support_v5<> m_doc_border_rank;
    doc_pos_mapper() = default;
    doc_pos_mapper(collection& col)
    {
        sdsl::load_from_file(m_doc_border, col.file_map[KEY_DBV]);
        m_doc_border_rank = sdsl::rank_support_v5<>(&m_doc_border);
    }

    inline size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = NULL, std::string name = "") const
    {
        using namespace sdsl;
        structure_tree_node* child = structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += m_doc_border.serialize(out,child,"doc border");
        written_bytes += m_doc_border_rank.serialize(out,child,"doc border rank");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    inline void load(std::istream& in)
    {
        m_doc_border.load(in);
        m_doc_border_rank.load(in,&m_doc_border);
    }
    uint64_t map_to_id(uint64_t pos) const
    {
        return m_doc_border_rank(pos);
    }
};
