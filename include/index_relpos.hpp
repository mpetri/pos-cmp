#pragma once

#include "list_types.hpp"
#include "index_invidx.hpp"
#include "doc_pos_mapper.hpp"

#include "easylogging++.h"

#pragma pack(1)
struct rel_metadata {
    uint64_t offset;
};

template<class t_pospl=optpfor_list<128,false>,class t_invidx = index_invidx<> >
class index_relpos
{
    public:
        using size_type = sdsl::int_vector<>::size_type;
        using plist_type = t_pospl;
        using doclist_type = typename t_invidx::id_list_type;
        using invidx_type = t_invidx;
        const std::string name = "RELPOS";
        std::string file_name;
    public:
        invidx_type m_docidx;
        size_t m_num_lists;
        std::vector<rel_metadata> m_meta_data;
        sdsl::bit_vector m_data;
        doc_pos_mapper m_dpm;
        bit_istream m_is;
    public:
        index_relpos(collection& col) : m_docidx(col), m_is(m_data)
        {
            file_name = col.path +"index/"+name+"-"+sdsl::util::class_to_hash(*this)+".idx";
            if (utils::file_exists(file_name)) {  // load
                LOG(INFO) << "LOAD from file '" << file_name << "'";
                std::ifstream ifs(file_name);
                load(ifs);
            } else { // construct
                {
                    LOG(INFO) << "CONSTRUCT relpos index";
                    // (2) create doc pos mapper
                    m_dpm = doc_pos_mapper(col);

                    // (3) pos index
                    bit_ostream bvo(m_data);
                    sdsl::int_vector_mapper<> POS(col.file_map[KEY_POSPL]);
                    sdsl::int_vector_mapper<> D(col.file_map[KEY_D]);
                    sdsl::bit_vector DBV;
                    sdsl::load_from_file(DBV,col.file_map[KEY_DBV]);
                    sdsl::int_vector_mapper<> C(col.file_map[KEY_C]);
                    m_num_lists = C.size();
                    m_meta_data.resize(m_num_lists);
                    size_t csum = C[0] + C[1];
                    for (size_t i=2; i<C.size(); i++) {
                        size_t n = C[i];
                        LOG_EVERY_N(C.size()/10, INFO) << "Construct relpos list " << i << " (" << n << ")";
                        auto begin = POS.begin()+csum;
                        auto end = begin + n;
                        std::vector<uint64_t> rel_pos(n);
                        std::copy(begin,end,rel_pos.begin());
                        auto prev = 0ULL;

                        auto first_one = 0ULL;
                        if (DBV[0]==1) first_one = 0ULL;
                        else first_one = sdsl::bits::next(DBV.data(),0ULL);
                        auto prev_doc_id = D.size()+1;
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
                        m_meta_data[i].offset = plist_type::create(bvo,rel_pos.begin(),rel_pos.end());
                        csum += n;
                    }
                    // prepare input stream
                    m_is.refresh();
                }

                LOG(INFO) << "STORE to file '" << file_name << "'";
                std::ofstream ofs(file_name);
                auto bytes = serialize(ofs);
                LOG(INFO) << "STORE space usage '" << file_name << ".html'";
                std::ofstream vofs(file_name+".html");
                sdsl::write_structure<sdsl::HTML_FORMAT>(vofs,*this,m_docidx);
                LOG(INFO) << "relpos index size : " << bytes / (1024*1024) << " MB";
            }
        }
        size_type serialize(std::ostream& out, sdsl::structure_tree_node* v=NULL, std::string name="") const
        {
            sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += sdsl::write_member(m_num_lists,out,child,"num plists");

            auto* listdata = sdsl::structure_tree::add_child(child, "list metadata","list metadata");
            out.write((const char*)m_meta_data.data(), m_meta_data.size()*sizeof(rel_metadata));
            written_bytes += m_meta_data.size()*sizeof(rel_metadata);
            sdsl::structure_tree::add_size(listdata, m_meta_data.size()*sizeof(rel_metadata));

            written_bytes += m_data.serialize(out,child,"list data");
            written_bytes += m_dpm.serialize(out,child,"doc pos mapper");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }
        void load(std::ifstream& ifs)
        {
            sdsl::read_member(m_num_lists,ifs);
            m_meta_data.resize(m_num_lists);
            ifs.read((char*)m_meta_data.data(),m_num_lists*sizeof(rel_metadata));
            m_data.load(ifs);
            m_dpm.load(ifs);
            m_is.refresh();
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
        intersection_result
        phrase_list(std::vector<uint64_t> ids) const
        {
            std::vector<offset_proxy_list<typename plist_type::list_type>> lists;
            size_type i = 0;
            for (const auto& id : ids) {
                lists.emplace_back(offset_proxy_list<typename plist_type::list_type>(list(id),i++));
            }
            return map_to_doc_ids(pos_intersect(lists));
        }
        template<class t_list>
        intersection_result
        map_to_doc_ids(const t_list& list) const
        {
            intersection_result res(list.size());
            auto itr = list.begin();
            auto end = list.end();
            auto prev_docid = m_dpm.map_to_id(*itr);
            ++itr;
            size_t n=0;
            while (itr != end) {
                auto pos = *itr;
                auto doc_id = m_dpm.map_to_id(pos);
                if (doc_id != prev_docid) {
                    res[n++] = prev_docid;
                    prev_docid = doc_id;
                }
                ++itr;
            }
            res[n++] = prev_docid;
            res.resize(n);
            return res;
        }
        uint64_t doc_start(uint64_t id) const
        {
            return m_dpm.doc_start(id);
        }
};