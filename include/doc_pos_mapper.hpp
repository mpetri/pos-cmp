#pragma once

#include <sdsl/int_vector.hpp>
#include <sdsl/rank_support_v5.hpp>
#include <string>
#include "doc_pos_mapper.hpp"

struct doc_pos_mapper {
    typedef typename sdsl::int_vector<>::size_type size_type;
    sdsl::bit_vector m_doc_border;
    sdsl::rank_support_v5<> m_doc_border_rank;
    sdsl::select_support_mcl<> m_doc_border_select;
    doc_pos_mapper() = default;
    doc_pos_mapper(const doc_pos_mapper& dp)
    {
        m_doc_border = dp.m_doc_border;
        m_doc_border_rank = dp.m_doc_border_rank;
        m_doc_border_rank.set_vector(&m_doc_border);
        m_doc_border_select = dp.m_doc_border_select;
        m_doc_border_select.set_vector(&m_doc_border);
    }
    doc_pos_mapper(doc_pos_mapper&& dp)
    {
        m_doc_border = std::move(dp.m_doc_border);
        m_doc_border_rank = std::move(dp.m_doc_border_rank);
        m_doc_border_rank.set_vector(&m_doc_border);
        m_doc_border_select = std::move(dp.m_doc_border_select);
        m_doc_border_select.set_vector(&m_doc_border);
    }
    doc_pos_mapper& operator=(doc_pos_mapper&& dp)
    {
        m_doc_border = std::move(dp.m_doc_border);
        m_doc_border_rank = std::move(dp.m_doc_border_rank);
        m_doc_border_rank.set_vector(&m_doc_border);
        m_doc_border_select = std::move(dp.m_doc_border_select);
        m_doc_border_select.set_vector(&m_doc_border);
        return *this;
    }
    doc_pos_mapper(collection& col)
    {
        sdsl::load_from_file(m_doc_border, col.file_map[KEY_DBV]);
        m_doc_border_rank = sdsl::rank_support_v5<>(&m_doc_border);
        m_doc_border_select = sdsl::select_support_mcl<>(&m_doc_border);
    }

    inline size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = NULL, std::string name = "") const
    {
        using namespace sdsl;
        structure_tree_node* child = structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += m_doc_border.serialize(out,child,"doc border");
        written_bytes += m_doc_border_rank.serialize(out,child,"doc border rank");
        written_bytes += m_doc_border_select.serialize(out,child,"doc border select");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    inline void load(std::istream& in)
    {
        m_doc_border.load(in);
        m_doc_border_rank.load(in,&m_doc_border);
        m_doc_border_select.load(in,&m_doc_border);
    }
    uint64_t map_to_id(uint64_t pos) const
    {
        return m_doc_border_rank(pos);
    }
    uint64_t doc_start(uint64_t id) const
    {
        if (id==0)
            return 0;
        else {
            return m_doc_border_select(id)+1;
        }
    }
};
