#pragma once

#include <limits>

#include "list_types.hpp"
#include "index_invidx.hpp"

#include "easylogging++.h"

#include <sdsl/dac_vector.hpp>

template<class t_pospl=optpfor_list<128,true>,class t_invidx = index_invidx<> >
class index_nextword
{
    public:
        using size_type = sdsl::int_vector<>::size_type;
        using plist_type = t_pospl;
        using doclist_type = typename t_invidx::id_list_type;
        using invidx_type = t_invidx;
        const std::string name = "NEXTWORD";
        std::string file_name;
    public:
        invidx_type m_docidx;
        size_t m_num_lists;
        sdsl::sd_vector<> m_meta_data;
        sdsl::select_support_sd<> m_meta_data_access;
        sdsl::bit_vector m_data;
        sdsl::sd_vector<> m_mapper;
        sdsl::rank_support_sd<> m_mapper_access;
        uint64_t m_sym_width;
    public:
        index_nextword(collection& col) : m_docidx(col)
        {
            file_name = col.path +"index/"+name+"-"+sdsl::util::class_to_hash(*this)+".idx";
            if (utils::file_exists(file_name)) {  // load
                LOG(INFO) << "LOAD from file '" << file_name << "'";
                std::ifstream ifs(file_name);
                load(ifs);
            } else { // construct
                // (2) pos index
                LOG(INFO) << "CONSTRUCT abs nextword index";
                bit_ostream bvo(m_data);
                sdsl::int_vector_mapper<> text(col.file_map[KEY_TEXT]);
                sdsl::int_vector_mapper<> SA(col.file_map[KEY_SA]);
                sdsl::int_vector_mapper<> SCC(col.file_map[KEY_SCC]);
                sdsl::int_vector_mapper<> CC(col.file_map[KEY_CC]);
                sdsl::int_vector_mapper<> C(col.file_map[KEY_C]);
                m_num_lists = CC.size();
                std::vector<uint64_t> meta_data(m_num_lists);
                size_t csum = 1; // skip 0
                m_sym_width = text.width();
                for (size_t i=0; i<CC.size(); i++) {
                    size_t n = SCC[i];
                    auto begin = SA.begin()+csum;
                    auto end = begin + n;
                    LOG_EVERY_N(250000, INFO) << "Construct nextword list " << i << " (" << n << ")";
                    std::vector<uint64_t> tmp(begin,end);
                    std::sort(tmp.begin(),tmp.end());
                    meta_data[i] = plist_type::create(bvo,tmp.begin(),tmp.end());
                    csum += n;
                }

                m_meta_data = sdsl::sd_vector<>(meta_data.begin(),meta_data.end());
                m_meta_data_access.set_vector(&m_meta_data);

                // (3) mapping of phrases to lists positions
                m_mapper = sdsl::sd_vector<>(CC.begin(),CC.end());
                m_mapper_access.set_vector(&m_mapper);

                // (4) write
                LOG(INFO) << "STORE to file '" << file_name << "'";
                std::ofstream ofs(file_name);
                auto bytes = serialize(ofs);
                LOG(INFO) << "STORE space usage '" << file_name << ".html'";
                std::ofstream vofs(file_name+".html");
                sdsl::write_structure<sdsl::HTML_FORMAT>(vofs,*this,m_docidx);
                LOG(INFO) << "abspos nextword index size : " << bytes / (1024*1024) << " MB";
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
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }
        void load(std::ifstream& ifs)
        {
            sdsl::read_member(m_sym_width,ifs);
            m_meta_data.load(ifs);
            m_meta_data_access.set_vector(&m_meta_data);
            m_num_lists = m_meta_data.size();
            m_mapper.load(ifs);
            m_mapper_access.set_vector(&m_mapper);
            m_data.load(ifs);
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
            bit_istream is(m_data);
            auto list_number = m_mapper_access(id);
            auto data_offset = m_meta_data_access(list_number+1);
            return plist_type::materialize(is,data_offset);
        }
        typename doclist_type::list_type
        doc_list(size_t i) const
        {
            return m_docidx.list(i).first;
        }
        std::vector<typename doclist_type::list_type>
        doc_lists(std::vector<uint64_t> ids)
        {
            std::vector<typename doclist_type::list_type> lists;
            for (const auto& id : ids) {
                lists.emplace_back(doc_list(id));
            }
            return lists;
        }
};