#pragma once

#include "list_types.hpp"
#include "index_invidx.hpp"

#include "easylogging++.h"

#pragma pack(1)
struct abs_metadata {
    uint64_t offset;
};


template<class t_pospl=optpfor_list<128,true>,class t_invidx = index_invidx<> >
class index_abspos
{
    public:
        using size_type = sdsl::int_vector<>::size_type;
        using plist_type = t_pospl;
        using invidx_type = t_invidx;
        const std::string name = "ABSPOS";
        std::string file_name;
    public:
        invidx_type m_docidx;
        size_t m_num_lists;
        std::vector<abs_metadata> m_meta_data;
        sdsl::bit_vector m_data;
    public:
        index_abspos(collection& col) : m_docidx(col)
        {
            file_name = col.path +"index/"+name+"-"+sdsl::util::class_to_hash(*this)+".idx";
            if (utils::file_exists(file_name)) {  // load
                LOG(INFO) << "LOAD from file '" << file_name << "'";
                std::ifstream ifs(file_name);
                load(ifs);
            } else { // construct
                {
                    // (2) pos index
                    LOG(INFO) << "CONSTRUCT abspos index";
                    bit_ostream bvo(m_data);
                    sdsl::int_vector_mapper<> POS(col.file_map[KEY_POSPL]);
                    sdsl::int_vector_mapper<> C(col.file_map[KEY_C]);
                    m_num_lists = C.size();
                    m_meta_data.resize(m_num_lists);
                    size_t csum = C[0] + C[1];
                    for (size_t i=2; i<C.size(); i++) {
                        size_t n = C[i];
                        LOG_EVERY_N(C.size()/10, INFO) << "Construct abspos list " << i << " (" << n << ")";
                        auto begin = POS.begin()+csum;
                        auto end = begin + n;
                        m_meta_data[i].offset = plist_type::create(bvo,begin,end);
                        csum += n;
                    }
                }
                LOG(INFO) << "STORE to file '" << file_name << "'";
                std::ofstream ofs(file_name);
                auto bytes = serialize(ofs);
                LOG(INFO) << "STORE space usage '" << file_name << "'";
                std::ofstream vofs(file_name+".html");
                sdsl::write_structure<sdsl::HTML_FORMAT>(vofs,*this,m_docidx);
                LOG(INFO) << "abspos index size : " << bytes / (1024*1024) << " MB";
            }
        }
        size_type serialize(std::ostream& out, sdsl::structure_tree_node* v=NULL, std::string name="") const
        {
            sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += sdsl::write_member(m_num_lists,out,child,"num plists");

            auto* listdata = sdsl::structure_tree::add_child(child, "list metadata","list metadata");
            out.write((const char*)m_meta_data.data(), m_meta_data.size()*sizeof(abs_metadata));
            written_bytes += m_meta_data.size()*sizeof(list_metadata);
            sdsl::structure_tree::add_size(listdata, m_meta_data.size()*sizeof(abs_metadata));

            written_bytes += m_data.serialize(out,child,"list data");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }
        void load(std::ifstream& ifs)
        {
            sdsl::read_member(m_num_lists,ifs);
            m_meta_data.resize(m_num_lists);
            ifs.read((char*)m_meta_data.data(),m_num_lists*sizeof(abs_metadata));
            m_data.load(ifs);
        }
        typename plist_type::iterator_pair
        list(size_t i) const
        {
            bit_istream is(m_data);
            return plist_type::iterators(is,m_meta_data[i].offset);
        }
};