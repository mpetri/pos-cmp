#pragma once

#include "list_types.hpp"
#include "rank_functions.hpp"
#include "range_iterators.hpp"
#include "intersection.hpp"

#include "easylogging++.h"

#pragma pack(1)
struct list_metadata {
    uint64_t id_offset;
    uint64_t freq_offset;
};

#pragma pack(1)
struct wand_metadata {
    double list_max_score;
    double max_doc_weight;
};

template<class t_pl_ids=optpfor_list<128,true>,
         class t_pl_freqs=optpfor_list<128,false>,
         class t_rank=rank_bm25<>>
class index_invidx
{
    public:
        using size_type = sdsl::int_vector<>::size_type;
        typedef t_pl_ids id_list_type;
        typedef t_pl_freqs freq_list_type;
        using rank_type = t_rank;
        const std::string name = "INVIDX";
        std::string file_name;
    private:
        size_t m_num_lists;
        std::vector<list_metadata> m_meta_data;
        std::vector<wand_metadata> m_wand_data;
        sdsl::bit_vector m_id_data;
        sdsl::bit_vector m_freq_data;
        rank_type m_ranker;
        doc_perm m_dp;
        bit_istream m_isi;
        bit_istream m_isf;
    private:
        template<class t_itr,class t_fitr>
        wand_metadata create_wand_data(t_itr ibegin,t_itr iend,t_fitr fbegin,t_fitr fend)
        {
            wand_metadata wm {0.0,0.0};
            auto F_t = std::accumulate(fbegin,fend,0);
            auto f_t = std::distance(ibegin,iend);
            auto tmp = ibegin;
            while (tmp != iend) {
                auto id = *tmp;
                auto f_dt = *fbegin;
                double W_d = m_ranker.doc_length(id);
                double doc_weight = m_ranker.calc_doc_weight(W_d);
                double score = m_ranker.calculate_docscore(1.0f,f_dt,f_t,F_t,W_d,true);
                wm.list_max_score = std::max(wm.list_max_score,score);
                wm.max_doc_weight = std::max(wm.max_doc_weight,doc_weight);
                ++tmp;
                ++fbegin;
            }
            return wm;
        }
    public:
        index_invidx(collection& col) : m_isi(m_id_data), m_isf(m_freq_data)
        {
            file_name = col.path +"index/"+name+"-"+sdsl::util::class_to_hash(*this)+".idx";
            if (utils::file_exists(file_name)) {  // load
                LOG(INFO) << "LOAD from file '" << file_name << "'";
                std::ifstream ifs(file_name);
                load(ifs);
            } else { // construct
                // (1) ranker loads doc lengths
                m_ranker = t_rank(col);
                // (2) doc permutation
                sdsl::load_from_file(m_dp,col.file_map[KEY_DOCPERM]);
                // (3) postings lists
                {
                    LOG(INFO) << "CONSTRUCT inverted index";
                    const sdsl::int_vector_mapper<0,std::ios_base::in> D(col.file_map[KEY_D]);
                    const sdsl::int_vector_mapper<0,std::ios_base::in> C(col.file_map[KEY_C]);
                    bit_ostream bvi(m_id_data);
                    bit_ostream bvf(m_freq_data);
                    m_num_lists = C.size();
                    m_meta_data.resize(m_num_lists);
                    m_wand_data.resize(m_num_lists);
                    size_t csum = C[0] + C[1];
                    for (size_t i=2; i<C.size(); i++) {
                        size_t n = C[i];
                        LOG_EVERY_N(C.size()/10, INFO) << "Construct invidx list " << i << " (" << n << ")";
                        std::vector<uint32_t> tmp(D.begin()+csum,D.begin()+csum+ n);
                        std::sort(tmp.begin(),tmp.end());

                        using itr_type = std::vector<uint32_t>::const_iterator;
                        id_range_adaptor<itr_type> id_range(tmp.begin(),tmp.end());
                        freq_range_adaptor<itr_type> freq_range(tmp.begin(),tmp.end());

                        // (a) ids
                        m_meta_data[i].id_offset = id_list_type::create(bvi,id_range.begin(),id_range.end());
                        // (b) freqs
                        m_meta_data[i].freq_offset = freq_list_type::create(bvf,freq_range.begin(),freq_range.end());

                        // (c) wand data
                        m_wand_data[i] = create_wand_data(id_range.begin(),id_range.end(),freq_range.begin(),freq_range.end());

                        csum += n;
                    }
                    LOG(INFO) << "Number of terms: " << C.size()-2;
                    LOG(INFO) << "Number of postings: " << csum - C[0] + C[1];
                }
                // prepare input streams
                m_isi.refresh(); m_isf.refresh();

                LOG(INFO) << "STORE to file '" << file_name << "'";
                std::ofstream ofs(file_name);
                auto bytes = serialize(ofs);
                std::ofstream vofs(file_name+".html");
                sdsl::write_structure<sdsl::HTML_FORMAT>(vofs,*this);

                LOG(INFO) << "inverted index size : " << bytes / (1024*1024) << " MB";
            }
        }
        size_type serialize(std::ostream& out, sdsl::structure_tree_node* v=NULL, std::string name="") const
        {
            sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += sdsl::write_member(m_num_lists,out,child,"num plists");
            written_bytes += m_ranker.serialize(out,child,"ranker");
            written_bytes += m_dp.serialize(out,child,"doc_perm");

            auto* wanddata = sdsl::structure_tree::add_child(child, "wand metadata","wand metadata");
            out.write((const char*)m_wand_data.data(), m_wand_data.size()*sizeof(wand_metadata));
            written_bytes += m_wand_data.size()*sizeof(wand_metadata);
            sdsl::structure_tree::add_size(wanddata, m_wand_data.size()*sizeof(wand_metadata));

            auto* listdata = sdsl::structure_tree::add_child(child, "list metadata","list metadata");
            out.write((const char*)m_meta_data.data(), m_meta_data.size()*sizeof(list_metadata));
            written_bytes += m_meta_data.size()*sizeof(list_metadata);
            sdsl::structure_tree::add_size(listdata, m_meta_data.size()*sizeof(list_metadata));

            written_bytes += m_id_data.serialize(out,child,"id data");
            written_bytes += m_freq_data.serialize(out,child,"freq data");

            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }
        void load(std::ifstream& ifs)
        {
            sdsl::read_member(m_num_lists,ifs);
            m_ranker.load(ifs);
            m_dp.load(ifs);
            m_wand_data.resize(m_num_lists);
            ifs.read((char*)m_wand_data.data(),m_num_lists*sizeof(wand_metadata));
            m_meta_data.resize(m_num_lists);
            ifs.read((char*)m_meta_data.data(),m_num_lists*sizeof(list_metadata));
            m_id_data.load(ifs);
            m_freq_data.load(ifs);
            m_isi.refresh();
            m_isf.refresh();
        }
        std::pair<typename id_list_type::list_type,typename freq_list_type::list_type>
        list(size_t i) const
        {
            return make_pair(id_list_type::materialize(m_isi,m_meta_data[i].id_offset),
                             freq_list_type::materialize(m_isf,m_meta_data[i].freq_offset)
                            );
        }
        intersection_result
        intersection(std::vector<uint64_t> ids) const
        {
            std::vector<typename id_list_type::list_type> lists;
            for (const auto& id : ids) {
                lists.emplace_back(id_list_type::materialize(m_isi,m_meta_data[id].id_offset));
            }
            return intersect(lists);
        }
};
