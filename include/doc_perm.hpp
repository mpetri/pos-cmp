#pragma once

#include <sdsl/int_vector.hpp>
#include <string>

struct doc_perm {
    typedef typename sdsl::int_vector<>::size_type size_type;
    sdsl::int_vector<> id2len; // doc id to length ordered id
    sdsl::int_vector<> len2id; // length ordered id to doc id
    bool is_identity;

    inline size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = NULL, std::string name = "") const
    {
        using namespace sdsl;
        structure_tree_node* child = structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += sdsl::write_member(is_identity,out,child,"is_identity");
        if (!is_identity) {
            written_bytes += id2len.serialize(out, child, "id2len");
            written_bytes += len2id.serialize(out, child, "len2id");
        }
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    inline void load(std::istream& in)
    {
        sdsl::read_member(is_identity,in);
        if (!is_identity) {
            id2len.load(in);
            len2id.load(in);
        }
    }
};
