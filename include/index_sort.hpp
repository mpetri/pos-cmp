#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <list>
#include <utility>

#include <sdsl/suffix_arrays.hpp>

using std::vector;

template<
    class t_csa = sdsl::csa_wt_int<sdsl::wt_int<sdsl::rrr_vector<63>>, 1000000, 1000000>>
class index_sort
{
    public:
        const std::string name = "SORT";
        std::string file_name;
        typedef t_csa                                       csa_type;
        typedef sdsl::int_vector<>                          d_type;
        typedef sdsl::int_vector<>::size_type               size_type;
    protected:
        size_type m_doc_cnt;
        csa_type  m_csa_full;
        d_type    m_d;
    public:
        index_sort(collection& col)
        {
            file_name = col.path +"index/"+name+"-"+sdsl::util::class_to_hash(*this)+".idx";
            if (utils::file_exists(file_name)) {  // load
                LOG(INFO) << "LOAD from file '" << file_name << "'";
                std::ifstream ifs(file_name);
                load(ifs);
            } else { // construct
                LOG(INFO) << "CONSTRUCT sort index";

                LOG(INFO) << "CONSTRUCT CSA";
                sdsl::cache_config cfg;
                cfg.delete_files = false;
                cfg.dir = col.path + "/tmp/";
                cfg.id = "TMP";
                cfg.file_map[sdsl::conf::KEY_SA] = col.file_map[KEY_SA];
                cfg.file_map[sdsl::conf::KEY_TEXT_INT] = col.file_map[KEY_TEXTPERM];
                construct(m_csa_full,col.file_map[KEY_TEXTPERM],cfg,0);

                LOG(INFO) << "CONSTRUCT D";
                sdsl::load_from_file(m_d,col.file_map[KEY_D]);

                LOG(INFO) << "STORE to file '" << file_name << "'";
                std::ofstream ofs(file_name);
                auto bytes = serialize(ofs);
                std::ofstream vofs(file_name+".html");
                sdsl::write_structure<sdsl::HTML_FORMAT>(vofs,*this);

                LOG(INFO) << "sort index size : " << bytes / (1024*1024) << " MB";
            }
        }

        size_type serialize(std::ostream& out,sdsl::structure_tree_node* v=NULL, std::string name="")const
        {
            sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_csa_full.serialize(out, child, "CSA");
            written_bytes += m_d.serialize(out, child, "D");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream& in)
        {
            m_csa_full.load(in);
            m_d.load(in);
        }

        void swap(index_sort& dr)
        {
            if (this != &dr) {
                m_csa_full.swap(dr.m_csa_full);
                m_d.swap(dr.m_d);
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
                size_t n = ep-sp+1;
                std::vector<uint64_t> tmp(n);
                for (size_t i=0; i<n; i++) tmp[i] = m_d[sp+i];
                std::sort(tmp.begin(),tmp.end());

                auto prev_id = tmp[0];
                auto freq = 1;
                for (size_t i=1; i<tmp.size(); i++) {
                    auto cur_id = tmp[i];
                    if (cur_id != prev_id) {
                        res.emplace_back(prev_id,freq);
                        prev_id = cur_id;
                        freq = 1;
                    } else {
                        freq++;
                    }
                }
                res.emplace_back(prev_id,freq);
                return res;
            }
        }
};
