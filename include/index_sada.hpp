#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <list>
#include <utility>

#include <sdsl/suffix_arrays.hpp>
#include <sdsl/rmq_support.hpp>

using std::vector;

template<
    class t_csa = sdsl::csa_wt_int<sdsl::wt_int<sdsl::hyb_vector<>>, 16, 1000000>,
    class t_range_min         = sdsl::rmq_succinct_sct<true>,
    class t_range_max         = sdsl::rmq_succinct_sct<false>,
    class t_doc_border        = sdsl::sd_vector<>,
    class t_doc_border_rank   = typename t_doc_border::rank_1_type,
    class t_doc_border_select = typename t_doc_border::select_1_type
    >
class index_sada
{
    public:
        const std::string name = "SADA";
        std::string file_name;
        typedef t_csa                                       csa_full_type;
        typedef t_range_min                                 range_min_type;
        typedef t_range_max                                 range_max_type;
        typedef t_doc_border                                doc_border_type;
        typedef t_doc_border_rank                           doc_border_rank_type;
        typedef t_doc_border_select                         doc_border_select_type;
        typedef sdsl::int_vector<>::size_type               size_type;
    protected:
        size_type m_doc_cnt;
        csa_full_type          m_csa_full;            // CSA build from the collection text
        std::vector<sdsl::int_vector<>>   m_doc_isa;  // array of inverse SAs. m_doc_isa[i] contains the ISA of document i
        range_min_type         m_rminq;                     // range minimum data structure build over an array Cprev
        range_max_type         m_rmaxq;             // range maximum data structure build over an array Cnext
        doc_border_type        m_doc_border;        // bitvector indicating the positions of the separators in the collection text
        doc_border_rank_type   m_doc_border_rank;   // rank data structure on m_doc_border
        doc_border_select_type m_doc_border_select; // select data structure on m_doc_border
        mutable sdsl::bit_vector     m_doc_rmin_marked;   // helper bitvector for search process
        mutable sdsl::bit_vector     m_doc_rmax_marked;   // helper bitvector for search process
    public:
        index_sada(collection& col)
        {
            file_name = col.path +"index/"+name+"-"+sdsl::util::class_to_hash(*this)+".idx";
            if (utils::file_exists(file_name)) {  // load
                LOG(INFO) << "LOAD from file '" << file_name << "'";
                std::ifstream ifs(file_name);
                load(ifs);
                m_doc_rmin_marked = sdsl::bit_vector(m_doc_cnt, 0);
                m_doc_rmax_marked = sdsl::bit_vector(m_doc_cnt, 0);
            } else { // construct
                LOG(INFO) << "CONSTRUCT sada index";

                LOG(INFO) << "CONSTRUCT CSA";
                sdsl::cache_config cfg;
                cfg.delete_files = false;
                cfg.dir = col.path + "/tmp/";
                cfg.id = "TMP";
                cfg.file_map[sdsl::conf::KEY_SA] = col.file_map[KEY_SA];
                cfg.file_map[sdsl::conf::KEY_TEXT_INT] = col.file_map[KEY_TEXTPERM];
                construct(m_csa_full,col.file_map[KEY_TEXTPERM],cfg,0);

                LOG(INFO) << "CONSTRUCT DOC BORDER RANK";
                sdsl::bit_vector dbv;
                sdsl::load_from_file(dbv,col.file_map[KEY_DBV]);
                m_doc_border = doc_border_type(dbv);
                m_doc_border_rank   = doc_border_rank_type(&m_doc_border);
                m_doc_border_select = doc_border_select_type(&m_doc_border);
                m_doc_cnt = m_doc_border_rank(m_doc_border.size());

                LOG(INFO) << "CONSTRUCT DOC ISA";
                {
                    m_doc_isa.resize(m_doc_cnt);
                    std::vector<uint64_t> doc_buffer;
                    sdsl::int_vector_mapper<> text(col.file_map[KEY_TEXTPERM]);
                    size_type doc_id = 0;
                    for (size_type i = 0; i < text.size(); ++i) {
                        if (1ULL == text[i]) {
                            if (doc_buffer.size() > 0) {
                                doc_buffer.push_back(0);
                                {
                                    sdsl::int_vector<> sa(doc_buffer.size(), 0, sdsl::bits::hi(doc_buffer.size())+1);
                                    sdsl::qsufsort::construct_sa(sa, doc_buffer);
                                    sdsl::util::bit_compress(sa);
                                    m_doc_isa[doc_id] = sa;
                                    for (size_type j = 0; j < doc_buffer.size(); ++j) {
                                        m_doc_isa[doc_id][sa[j]] = j;
                                    }
                                }
                            }
                            ++doc_id;
                            doc_buffer.clear();
                        } else {
                            doc_buffer.push_back(text[i]);
                        }
                    }
                }

                {
                    sdsl::int_vector_mapper<> D(col.file_map[KEY_D]);
                    {
                        LOG(INFO) << "CONSTRUCT CPrev";
                        sdsl::int_vector<> Cprev(D.size(), 0, sdsl::bits::hi(D.size())+1);
                        sdsl::int_vector<> last_occ(m_doc_cnt+1, 0, sdsl::bits::hi(D.size())+1);
                        for (size_type i = 0; i < D.size(); ++i) {
                            size_type doc = D[i];
                            Cprev[i]      = last_occ[doc];
                            last_occ[doc] = i;
                        }
                        m_rminq = range_min_type(&Cprev);
                    }
                    {
                        LOG(INFO) << "CONSTRUCT CNext";
                        sdsl::int_vector<> Cnext(D.size(), 0, sdsl::bits::hi(D.size())+1);
                        sdsl::int_vector<> last_occ(m_doc_cnt+1, D.size(), sdsl::bits::hi(D.size())+1);
                        for (size_type i = 0, j = D.size()-1; i < D.size(); ++i, --j) {
                            size_type doc = D[j];
                            Cnext[j]      = last_occ[doc];
                            last_occ[doc] = j;
                        }
                        m_rmaxq = range_max_type(&Cnext);
                    }
                }
                m_doc_rmin_marked = sdsl::bit_vector(m_doc_cnt, 0);
                m_doc_rmax_marked = sdsl::bit_vector(m_doc_cnt, 0);

                LOG(INFO) << "STORE to file '" << file_name << "'";
                std::ofstream ofs(file_name);
                auto bytes = serialize(ofs);
                std::ofstream vofs(file_name+".html");
                sdsl::write_structure<sdsl::HTML_FORMAT>(vofs,*this);

                LOG(INFO) << "sada index size : " << bytes / (1024*1024) << " MB";
            }
        }

        size_type serialize(std::ostream& out,sdsl::structure_tree_node* v=NULL, std::string name="")const
        {
            sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += sdsl::write_member(m_doc_cnt, out, child, "doc_cnt");
            written_bytes += m_csa_full.serialize(out, child, "csa_full");
            written_bytes += sdsl::serialize_vector(m_doc_isa, out, child, "doc_isa");
            written_bytes += m_rminq.serialize(out, child, "rminq");
            written_bytes += m_rmaxq.serialize(out, child, "rmaxq");
            written_bytes += m_doc_border.serialize(out, child, "doc_border");
            written_bytes += m_doc_border_rank.serialize(out, child, "doc_border_rank");
            written_bytes += m_doc_border_select.serialize(out, child, "doc_border_select");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream& in)
        {
            sdsl::read_member(m_doc_cnt, in);
            m_csa_full.load(in);
            m_doc_isa.resize(m_doc_cnt);
            sdsl::load_vector(m_doc_isa, in);
            m_rminq.load(in);
            m_rmaxq.load(in);
            m_doc_border.load(in);
            m_doc_border_rank.load(in);
            m_doc_border_rank.set_vector(&m_doc_border);
            m_doc_border_select.load(in);
            m_doc_border_select.set_vector(&m_doc_border);
        }

        void swap(index_sada& dr)
        {
            if (this != &dr) {
                std::swap(m_doc_cnt, dr.m_doc_cnt);
                m_csa_full.swap(dr.m_csa_full);
                m_doc_isa.swap(dr.m_doc_isa);
                m_rminq.swap(dr.m_rminq);
                m_rmaxq.swap(dr.m_rmaxq);
                m_doc_border.swap(dr.m_doc_border);
                sdsl::util::swap_support(m_doc_border_rank, dr.m_doc_border_rank,
                                         &m_doc_border, &(dr.m_doc_border));
                sdsl::util::swap_support(m_doc_border_select, dr.m_doc_border_select,
                                         &m_doc_border, &(dr.m_doc_border));
                m_doc_rmin_marked.swap(dr.m_doc_rmin_marked);
                m_doc_rmax_marked.swap(dr.m_doc_rmax_marked);
            }
        }

        docfreq_result
        phrase_list(std::vector<uint64_t> ids) const
        {
            docfreq_result ires;
            size_type sp=1, ep=0;
            if (0 == sdsl::backward_search(m_csa_full, 0, m_csa_full.size()-1, ids.begin(),ids.end(), sp, ep)) {
                return ires;
            } else {
                std::vector<size_type> suffixes;
                get_lex_smallest_suffixes(sp, ep, suffixes);
                get_lex_largest_suffixes(sp, ep, suffixes);
                std::sort(suffixes.begin(), suffixes.end());
                for (size_type i=0; i < suffixes.size(); i+=2) {
                    size_type suffix_1 = suffixes[i];
                    size_type suffix_2 = suffixes[i+1];
                    size_type doc                 = m_doc_border_rank(suffix_1+1);
                    m_doc_rmin_marked[doc]        = 0;  // reset marking, which was set in get_lex_smallest_suffixes
                    m_doc_rmax_marked[doc]        = 0;  //                                 get_lex_largest_suffixes

                    if (suffix_1 == suffix_2) {  // if pattern occurs exactly once
                        ires.emplace_back(doc,1); // add the #occurrence
                    } else {
                        size_type doc_begin = doc ? m_doc_border_select(doc) + 1 : 0;
                        size_type doc_sp    = m_doc_isa[doc][ suffix_1 - doc_begin ];
                        size_type doc_ep    = m_doc_isa[doc][ suffix_2 - doc_begin ];
                        if (doc_sp > doc_ep) {
                            std::swap(doc_sp, doc_ep);
                        }
                        ires.emplace_back(doc, doc_ep - doc_sp + 1);
                    }
                }
                return ires;
            }
        }

        void get_lex_smallest_suffixes(size_type sp, size_type ep, vector<size_type>& suffixes) const
        {
            using lex_range_t = std::pair<size_type,size_type>;
            std::stack<lex_range_t> stack;
            stack.emplace(sp,ep);
            while (!stack.empty()) {
                auto range = stack.top();
                stack.pop();
                size_type rsp = std::get<0>(range);
                size_type rep = std::get<1>(range);
                if (rsp <= rep) {
                    size_type min_idx = m_rminq(rsp,rep);
                    size_type suffix  = m_csa_full[min_idx];
                    size_type doc     = m_doc_border_rank(suffix+1);

                    if (!m_doc_rmin_marked[doc]) {
                        suffixes.push_back(suffix);
                        m_doc_rmin_marked[doc] = 1;
                        stack.emplace(min_idx+1,rep);
                        stack.emplace(rsp,min_idx-1); // min_idx != 0, since `\0` is appended to string
                    }
                }
            }
        }

        void get_lex_largest_suffixes(size_type sp, size_type ep, vector<size_type>& suffixes) const
        {
            using lex_range_t = std::pair<size_type,size_type>;
            std::stack<lex_range_t> stack;
            stack.emplace(sp,ep);
            while (!stack.empty()) {
                auto range = stack.top();
                stack.pop();
                size_type rsp = std::get<0>(range);
                size_type rep = std::get<1>(range);
                if (rsp <= rep) {
                    size_type max_idx = m_rmaxq(rsp,rep);
                    size_type suffix  = m_csa_full[max_idx];
                    size_type doc     = m_doc_border_rank(suffix+1);

                    if (!m_doc_rmax_marked[doc]) {
                        suffixes.push_back(suffix);
                        m_doc_rmax_marked[doc] = 1;
                        stack.emplace(rsp,max_idx - 1); // max_idx != 0, since `\0` is appended to string
                        stack.emplace(max_idx+1,rep);
                    }
                }
            }
        }
};
