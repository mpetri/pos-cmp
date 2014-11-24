#pragma once

#include <sdsl/int_vector.hpp>
#include "collection.hpp"

template<uint32_t t_k1=120,uint32_t t_b=75>
struct rank_bm25 {
    using size_type = sdsl::int_vector<>::size_type;
    static const double k1;
    static const double b;
    static const double epsilon_score;
    size_t num_docs;
    size_t num_terms;
    double avg_doc_len;
    sdsl::int_vector<> doc_lengths;

    static std::string name()
    {
        return "bm25";
    }

    rank_bm25() {}

    rank_bm25& operator=(const rank_bm25&) = default;

    rank_bm25(collection& col)
    {
        load_from_file(doc_lengths,col.file_map[KEY_DOCLEN]);
        num_docs = doc_lengths.size();
        {
            sdsl::int_vector_mapper<> text(col.file_map[KEY_TEXT]);
            num_terms = text.size() - num_docs;
        }
        avg_doc_len = (double)num_terms / (double)num_docs;

        LOG(INFO) << "num docs : " << num_docs;
        LOG(INFO) << "avg doc len : " << avg_doc_len;
    }
    double doc_length(size_t doc_id) const
    {
        return (double) doc_lengths[doc_id];
    }
    double calc_doc_weight(double) const
    {
        return 0;
    }
    double calculate_docscore(const double f_qt,const double f_dt,const double f_t,
                              const double ,const double W_d,bool) const
    {
        double w_qt = std::max(epsilon_score, log((num_docs - f_t + 0.5) / (f_t+0.5)) * f_qt);
        double K_d = k1*((1-b) + (b*(W_d/avg_doc_len)));
        double w_dt = ((k1+1)*f_dt) / (K_d + f_dt);
        return w_dt*w_qt;
    }
    void load(std::ifstream& ifs)
    {
        sdsl::read_member(num_docs,ifs);
        sdsl::read_member(num_terms,ifs);
        sdsl::read_member(avg_doc_len,ifs);
        LOG(INFO) << "num docs : " << num_docs;
        LOG(INFO) << "num terms : " << num_terms;
        LOG(INFO) << "avg doc len : " << avg_doc_len;
        doc_lengths.load(ifs);
    }
    size_type serialize(std::ostream& out, sdsl::structure_tree_node* v=NULL, std::string name="") const
    {
        sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += sdsl::write_member(num_docs,out,child,"num_docs");
        written_bytes += sdsl::write_member(num_terms,out,child,"num_terms");
        written_bytes += sdsl::write_member(avg_doc_len,out,child,"avg_doc_len");
        written_bytes += doc_lengths.serialize(out,child,"doc lengths");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }
};

template<uint32_t t_k1,uint32_t t_b>
const double rank_bm25<t_k1,t_b>::k1 = (double)t_k1/100.0;

template<uint32_t t_k1,uint32_t t_b>
const double rank_bm25<t_k1,t_b>::b = (double)t_b/100.0;

template<uint32_t t_k1,uint32_t t_b>
const double rank_bm25<t_k1,t_b>::epsilon_score = 1e-6;

