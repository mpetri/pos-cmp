#pragma once

#include "list_types.hpp"
#include "index_invidx.hpp"
#include "doc_pos_mapper.hpp"
#include "intersection.hpp"

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
        using doclist_type = typename t_invidx::id_list_type;
        using invidx_type = t_invidx;
        const std::string name = "ABSPOS";
        std::string file_name;
    public:
        invidx_type m_docidx;
        size_t m_num_lists;
        std::vector<abs_metadata> m_meta_data;
        sdsl::bit_vector m_data;
        doc_pos_mapper m_dpm;
        bit_istream m_is;
    public:
        index_abspos(collection& col) : m_docidx(col), m_is(m_data)
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
                    // prepare input stream
                    m_is.refresh();
                }

                // (3) create doc pos mapper
                m_dpm = doc_pos_mapper(col);

                LOG(INFO) << "STORE to file '" << file_name << "'";
                std::ofstream ofs(file_name);
                auto bytes = serialize(ofs);
                LOG(INFO) << "STORE space usage '" << file_name << ".html'";
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
            written_bytes += m_meta_data.size()*sizeof(abs_metadata);
            sdsl::structure_tree::add_size(listdata, m_meta_data.size()*sizeof(abs_metadata));

            written_bytes += m_data.serialize(out,child,"list data");
            written_bytes += m_dpm.serialize(out,child,"doc pos mapper");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }
        void load(std::ifstream& ifs)
        {
            sdsl::read_member(m_num_lists,ifs);
            m_meta_data.resize(m_num_lists);
            ifs.read((char*)m_meta_data.data(),m_num_lists*sizeof(abs_metadata));
            m_data.load(ifs);
            m_dpm.load(ifs);
        }
        typename plist_type::list_type
        list(size_t i) const
        {
            return plist_type::materialize(m_is,m_meta_data[i].offset);
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
            std::vector<typename plist_type::list_type> plists;
            std::vector<offset_proxy_list<typename plist_type::list_type>> lists;
            size_type i = 0;
            for (const auto& id : ids) {
                plists.emplace_back(list(id));
                lists.emplace_back(offset_proxy_list<typename plist_type::list_type>(plists.back(),i++));
            }
            return map_to_doc_ids(pos_intersect(lists));
        }
        intersection_result
        phrase_positions(std::vector<uint64_t> ids) const
        {
            std::vector<typename plist_type::list_type> plists;
            std::vector<offset_proxy_list<typename plist_type::list_type>> lists;
            size_type i = 0;
            for (const auto& id : ids) {
                plists.emplace_back(list(id));
                lists.emplace_back(offset_proxy_list<typename plist_type::list_type>(plists.back(),i++));
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
            //std::cout << "pos = " << *itr << " docid = " << doc_id << std::endl;
            ++itr;
            size_t freq = 1;
            while (itr != end) {
                auto pos = *itr;
                auto doc_id = m_dpm.map_to_id(pos);
                //std::cout << "pos = " << pos << " docid = " << doc_id << std::endl;
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
