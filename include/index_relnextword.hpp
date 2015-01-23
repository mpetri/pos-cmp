#pragma once

#include <limits>

#include "list_types.hpp"
#include "index_invidx.hpp"

#include "easylogging++.h"

#include <sdsl/dac_vector.hpp>

template<class t_pospl=optpfor_list<128,false>,class t_invidx = index_invidx<> >
class index_relnextword
{
    public:
        using size_type = sdsl::int_vector<>::size_type;
        using plist_type = t_pospl;
        using doclist_type = typename t_invidx::id_list_type;
        using invidx_type = t_invidx;
        const std::string name = "RELNEXTWORD";
        std::string file_name;
    public:
        invidx_type m_docidx;
        size_t m_num_lists;
        sdsl::sd_vector<> m_meta_data;
        sdsl::select_support_sd<> m_meta_data_access;
        sdsl::bit_vector m_data;
        sdsl::sd_vector<> m_mapper;
        sdsl::rank_support_sd<> m_mapper_access;
        doc_pos_mapper m_dpm;
        uint64_t m_sym_width;
        bit_istream m_is;
    public:
        index_relnextword(collection& col) : m_docidx(col), m_is(m_data)
        {
            file_name = col.path +"index/"+name+"-"+sdsl::util::class_to_hash(*this)+".idx";
            if (utils::file_exists(file_name)) {  // load
                LOG(INFO) << "LOAD from file '" << file_name << "'";
                {
                    const sdsl::int_vector_mapper<0,std::ios_base::in> CC(col.file_map[KEY_CC]);
                    m_num_lists = CC.size();
                }
                std::ifstream ifs(file_name);
                load(ifs);
            } else { // construct
                LOG(INFO) << "CONSTRUCT rel nextword index";
                // (1) create doc pos mapper
                m_dpm = doc_pos_mapper(col);

                // (2) pos index
                std::vector<uint64_t> meta_data;
                const sdsl::int_vector_mapper<0,std::ios_base::in> CC(col.file_map[KEY_CC]);
                {
                    bit_ostream bvo(m_data);
                    {
                        const sdsl::int_vector_mapper<0,std::ios_base::in> text(col.file_map[KEY_TEXT]);
                        m_sym_width = text.width();
                    }
                    const sdsl::int_vector_mapper<0,std::ios_base::in> SA(col.file_map[KEY_SA]);
                    const sdsl::int_vector_mapper<0,std::ios_base::in> SCC(col.file_map[KEY_SCC]);
                    const sdsl::int_vector_mapper<0,std::ios_base::in> C(col.file_map[KEY_C]);
                    sdsl::bit_vector DBV;
                    sdsl::load_from_file(DBV,col.file_map[KEY_DBV]);
                    m_num_lists = CC.size();
                    meta_data.resize(m_num_lists);
                    size_t csum = 1; // skip 0
                    for (size_t i=0; i<CC.size(); i++) {
                        size_t n = SCC[i];
                        auto begin = SA.begin()+csum;
                        auto end = begin + n;
                        LOG_EVERY_N(250000, INFO) << "Construct rel nextword list " << i << " (" << n << ")";
                        std::vector<uint64_t> tmp(begin,end);
                        std::sort(tmp.begin(),tmp.end());

                        // map to begin of doc
                        std::vector<uint64_t> rel_pos(n);
                        std::copy(tmp.begin(),tmp.end(),rel_pos.begin());
                        auto prev = 0ULL;

                        auto first_one = 0ULL;
                        if (DBV[0]==1) first_one = 0ULL;
                        else first_one = sdsl::bits::next(DBV.data(),0ULL);
                        auto prev_doc_id = SA.size()+1;
                        for (size_t j=0; j<rel_pos.size(); j++) {
                            auto p = rel_pos[j];
                            auto cur_doc_id = m_dpm.map_to_id(p);
                            auto rpos = 0ULL;
                            if (cur_doc_id != prev_doc_id) {
                                // delta to the start of the doc
                                if (p > first_one) {
                                    auto one_pos = sdsl::bits::prev(DBV.data(),p);
                                    rpos = p - (one_pos+1ULL);
                                } else {
                                    rpos = p;
                                }
                            } else {
                                // delta to the prev item
                                rpos = p - prev;
                            }
                            rel_pos[j] = rpos;
                            prev = p;
                            prev_doc_id = cur_doc_id;
                        }
                        meta_data[i] = plist_type::create(bvo,rel_pos.begin(),rel_pos.end());
                        csum += n;
                    }
                }
                m_is.refresh(); // init input stream

                m_meta_data = sdsl::sd_vector<>(meta_data.begin(),meta_data.end());
                m_meta_data_access.set_vector(&m_meta_data);

                // (3) mapping of phrases to lists positions
                m_mapper = sdsl::sd_vector<>(CC.begin(),CC.end());
                m_mapper_access.set_vector(&m_mapper);

                // (5) write
                LOG(INFO) << "STORE to file '" << file_name << "'";
                std::ofstream ofs(file_name);
                auto bytes = serialize(ofs);
                LOG(INFO) << "STORE space usage '" << file_name << ".html'";
                std::ofstream vofs(file_name+".html");
                sdsl::write_structure<sdsl::HTML_FORMAT>(vofs,*this,m_docidx);
                LOG(INFO) << "relpos nextword index size : " << bytes / (1024*1024) << " MB";
            }
        }
        size_type serialize(std::ostream& out, sdsl::structure_tree_node* v=NULL, std::string name="") const
        {
            sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += sdsl::write_member(m_sym_width,out,child,"sym width");
            written_bytes += m_meta_data.serialize(out,child,"list metadata");
            written_bytes += m_mapper.serialize(out,child,"phrase mapper");
            written_bytes += m_data.serialize(out,child,"list data");
            written_bytes += m_dpm.serialize(out,child,"doc pos mapper");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }
        void load(std::ifstream& ifs)
        {
            sdsl::read_member(m_sym_width,ifs);
            m_meta_data.load(ifs);
            m_meta_data_access.set_vector(&m_meta_data);
            m_mapper.load(ifs);
            m_mapper_access.set_vector(&m_mapper);
            m_data.load(ifs);
            m_dpm.load(ifs);
            m_is.refresh(); // init input stream
        }
        bool exists(uint64_t id1,uint64_t id2) const
        {
            uint64_t id = (id1 << m_sym_width) + id2;
            return exists(id);
        }
        bool exists(uint64_t id) const
        {
            return (id < m_mapper.size() && m_mapper[id] == 1);
        }
        typename plist_type::list_type
        list(uint64_t id1,uint64_t id2) const
        {
            uint64_t id = (id1 << m_sym_width) + id2;
            return list(id);
        }
        typename plist_type::list_type
        list(uint64_t id) const
        {
            auto list_number = m_mapper_access(id);
            auto data_offset = m_meta_data_access(list_number+1);
            return plist_type::materialize(m_is,data_offset);
        }
        typename doclist_type::list_type
        doc_list(size_t i) const
        {
            return m_docidx.list(i).first;
        }
        std::vector<typename doclist_type::list_type>
        doc_lists(std::vector<uint64_t> ids) const
        {
            std::vector<typename doclist_type::list_type> lists;
            for (const auto& id : ids) {
                lists.emplace_back(doc_list(id));
            }
            return lists;
        }
        docfreq_result
        phrase_list(std::vector<uint64_t> ids) const
        {
            if (ids.size() == 2) {
                return map_to_doc_ids(list(ids[0],ids[1]));
            }
            std::vector<typename plist_type::list_type> plists;
            std::vector<offset_proxy_list<typename plist_type::list_type>> lists;
            auto m = ids.size();
            if (m%2!=0) m--;
            for (size_t i=0; i<m; i+=2) {
                plists.emplace_back(list(ids[i],ids[i+1]));
                lists.emplace_back(offset_proxy_list<typename plist_type::list_type>(plists.back(),i));
            }
            if (ids.size()%2!=0) {
                plists.emplace_back(list(ids[ids.size()-2],ids[ids.size()-1]));
                lists.emplace_back(offset_proxy_list<typename plist_type::list_type>(plists.back(),ids.size()-2));
            }
            auto res = pos_intersect(lists);
            return map_to_doc_ids(res);
        }
        intersection_result
        phrase_positions(std::vector<uint64_t> ids) const
        {
            if (ids.size() == 2) {
                auto lst = list(ids[0],ids[1]);
                intersection_result res(lst.size());
                auto itr = lst.begin();
                auto end = lst.end();
                size_t n=0;
                while (itr != end) {
                    res[n++] = *itr;
                    ++itr;
                }
                return res;
            }
            std::vector<offset_proxy_list<typename plist_type::list_type>> lists;
            std::vector<typename plist_type::list_type> plists;
            auto m = ids.size();
            if (m%2!=0) m--;
            for (size_t i=0; i<m; i+=2) {
                plists.emplace_back(list(ids[i],ids[i+1]));
                lists.emplace_back(offset_proxy_list<typename plist_type::list_type>(plists.back(),i));
            }
            if (ids.size()%2!=0) {
                plists.emplace_back(list(ids[ids.size()-2],ids[ids.size()-1]));
                lists.emplace_back(offset_proxy_list<typename plist_type::list_type>(plists.back(),ids.size()-2));
            }
            return pos_intersect(lists);
        }
        template<class t_list>
        docfreq_result
        map_to_doc_ids(const t_list& list) const
        {
            docfreq_result res;
            auto itr = list.begin();
            auto end = list.end();
            auto prev_docid = m_dpm.map_to_id(*itr);
            ++itr;
            size_t freq = 1;
            while (itr != end) {
                auto pos = *itr;
                auto doc_id = m_dpm.map_to_id(pos);
                if (doc_id != prev_docid) {
                    res.emplace_back(prev_docid,freq);
                    freq = 1;
                    prev_docid = doc_id;
                } else {
                    freq++;
                }
                ++itr;
            }
            res.emplace_back(prev_docid,freq);
            return res;
        }
};
