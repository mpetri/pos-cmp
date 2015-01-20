#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <list>
#include <utility>

#include <sdsl/suffix_arrays.hpp>

using std::vector;

template<
    class t_csa = sdsl::csa_wt_int<sdsl::wt_int<sdsl::rrr_vector<63>>, 1000000, 1000000>,
    class t_wtd = sdsl::wt_int<sdsl::hyb_vector<>>
    >
class index_wt
{
    public:
        const std::string name = "WT";
        std::string file_name;
        typedef t_csa                                       csa_type;
        typedef t_wtd           							wtd_type;
        typedef sdsl::int_vector<>::size_type               size_type;
    protected:
        size_type m_doc_cnt;
        csa_type  m_csa_full;
        wtd_type    m_wtd;
    public:
        index_wt(collection& col)
        {
            file_name = col.path +"index/"+name+"-"+sdsl::util::class_to_hash(*this)+".idx";
            if (utils::file_exists(file_name)) {  // load
                LOG(INFO) << "LOAD from file '" << file_name << "'";
                std::ifstream ifs(file_name);
                load(ifs);
            } else { // construct
                LOG(INFO) << "CONSTRUCT wt index";

                LOG(INFO) << "CONSTRUCT CSA";
                sdsl::cache_config cfg;
                cfg.delete_files = false;
                cfg.dir = col.path + "/tmp/";
                cfg.id = "TMP";
                cfg.file_map[sdsl::conf::KEY_SA] = col.file_map[KEY_SA];
                cfg.file_map[sdsl::conf::KEY_TEXT_INT] = col.file_map[KEY_TEXTPERM];
                construct(m_csa_full,col.file_map[KEY_TEXTPERM],cfg,0);

                LOG(INFO) << "CONSTRUCT WTD";
                sdsl::int_vector_buffer<> D(col.file_map[KEY_D]);
                m_wtd = wtd_type(D,D.size());

                LOG(INFO) << "STORE to file '" << file_name << "'";
                std::ofstream ofs(file_name);
                auto bytes = serialize(ofs);
                std::ofstream vofs(file_name+".html");
                sdsl::write_structure<sdsl::HTML_FORMAT>(vofs,*this);

                LOG(INFO) << "wt index size : " << bytes / (1024*1024) << " MB";
            }
        }

        size_type serialize(std::ostream& out,sdsl::structure_tree_node* v=NULL, std::string name="")const
        {
            sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_csa_full.serialize(out, child, "CSA");
            written_bytes += m_wtd.serialize(out, child, "WTD");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream& in)
        {
            m_csa_full.load(in);
            m_wtd.load(in);
        }

        void swap(index_wt& dr)
        {
            if (this != &dr) {
                m_csa_full.swap(dr.m_csa_full);
                m_wtd.swap(dr.m_wtd);
            }
        }

        docfreq_result
        phrase_list(std::vector<uint64_t> ids) const
        {
            docfreq_result res;
            size_type sp=1, ep=0;
            if (0 == sdsl::backward_search(m_csa_full, 0, m_csa_full.size()-1,ids.begin(),ids.end(), sp, ep)) {
                return res;
            } else {
                std::vector<typename wtd_type::value_type> docs(ep-sp+1);
                std::vector<typename wtd_type::value_type> rdoc_sp(ep-sp+1);
                std::vector<typename wtd_type::value_type> rdoc_ep(ep-sp+1);
                size_t num_docs = 0;
                sdsl::interval_symbols(m_wtd,sp,ep+1,num_docs,docs,rdoc_sp,rdoc_ep);
                res.resize(num_docs);
                for (size_t i=0; i<num_docs; i++) {
                    res[i] = make_pair(docs[i],rdoc_ep[i]-rdoc_sp[i]);
                }
                return res;
            }
        }

        intersection_result
        doc_intersection(std::vector<uint64_t> ids) const
        {
            intersection_result res(1);
            std::vector<std::pair<size_type,size_type>> ranges;
            for (const auto& id: ids) {
                size_type sp=1, ep=0;
                std::vector<uint64_t> tmpids(1);
                tmpids[0] = id;
                if (0 == sdsl::backward_search(m_csa_full, 0, m_csa_full.size()-1,tmpids.begin(),tmpids.end(), sp, ep)) {
                    return res;
                } else {
                    ranges.emplace_back(sp,ep);
                }
            }
            auto result = sdsl::intersect(m_wtd,ranges);
            res.resize(result.size());
            for (size_t i=0; i<result.size(); i++) res[i] = result[i].first;
            return res;
        }
};
