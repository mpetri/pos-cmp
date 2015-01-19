#pragma once

#include <iterator>

#include "utils.hpp"
#include "util.h"
#include "memutil.h"
#include "codecs.h"
#include "codecfactory.h"
#include "bitpacking.h"
#include "simdfastpfor.h"
#include "deltautil.h"

#include "list_basics.hpp"
#include "eliasfano_list.hpp"
#include "eliasfano_skip_list.hpp"
#include "bitvector_list.hpp"

enum class uef_blocktype
{
    EF, BV, FULL
};


uef_blocktype
determine_block_type(size_t m,size_t n)
{
    if (n==m-1) return uef_blocktype::FULL;
    auto ef_bits = eliasfano_list<true,true>::estimate_size(m,n);
    auto bv_bits = bitvector_list<>::estimate_size(m,n);
    if (bv_bits < ef_bits) return uef_blocktype::BV;
    return uef_blocktype::EF;
}


template<uint16_t t_block_size = 128>
class uniform_ef_iterator : public std::iterator<std::random_access_iterator_tag,uint64_t,std::ptrdiff_t>
{
    public:
        using size_type = sdsl::int_vector<>::size_type;
        using top_list_type = eliasfano_skip_list<64,true,false>;
    private:
        uint64_t m_size;
        uint64_t m_num_blocks;
        const uint64_t* m_data = nullptr;
        const uint64_t* m_blockstart = nullptr;
        const bit_istream* m_iss = nullptr;
    private:
        mutable value_type m_cur_elem = 0;
        mutable size_type m_last_accessed_offset = std::numeric_limits<uint64_t>::max();
        size_type m_cur_offset = 0;
        mutable size_type m_cur_block_value_offset = 0;
        mutable size_type m_cur_block_universe;
        mutable typename top_list_type::iterator_type m_top_itr;
        mutable typename top_list_type::iterator_type m_top_end;
        mutable size_type m_prev_top = 0;
        mutable uef_blocktype m_cur_block_type;
    private:
        mutable ef_iterator<true,true> m_ef_block_itr;
        mutable ef_iterator<true,true> m_ef_block_end;
        mutable bv_iterator<true> m_bv_block_itr;
        mutable bv_iterator<true> m_bv_block_end;
    public:
        uniform_ef_iterator() = default;
        uniform_ef_iterator(const uniform_ef_iterator& pi) = default;
        uniform_ef_iterator(uniform_ef_iterator&& pi) = default;
        uniform_ef_iterator& operator=(uniform_ef_iterator&& pi) = default;
        uniform_ef_iterator& operator=(const uniform_ef_iterator& pi) = default;
    public:
        uniform_ef_iterator(const bit_istream& is,size_t start_offset,bool end) : m_iss(&is)
        {
            m_data = is.data();
            is.seek(start_offset);
            m_size = is.decode<coder::elias_gamma>();
            m_num_blocks = m_size/t_block_size;
            if (m_size%t_block_size!=0) m_num_blocks++;
            m_cur_offset = end ? m_size : 0;

            if (m_num_blocks == 1) {
                auto top_offset = is.tellg();
                auto top_list = top_list_type::materialize(is,top_offset);
                m_top_itr = top_list.begin();
                m_top_end = top_list.end();
            } else {
                is.align64();
                m_blockstart = is.cur_data();
                is.skip(m_num_blocks*64);

                // prepare top lvl list
                auto top_offset = is.tellg();
                auto top_list = top_list_type::materialize(is,top_offset);
                m_top_itr = top_list.begin();
                m_top_end = top_list.end();

                // decode first block
                m_prev_top = 0;
                m_cur_block_universe = *m_top_itr;
                m_cur_block_value_offset = 0;
                if (m_cur_offset != m_size) {
                    auto items_in_block = (m_size - m_cur_offset) < t_block_size ? m_size - m_cur_offset : t_block_size;
                    m_cur_block_type = determine_block_type(items_in_block,m_cur_block_universe);
                    if (m_cur_block_type == uef_blocktype::BV) {
                        auto list = bitvector_list<true>::materialize(*m_iss,m_blockstart[0],items_in_block,m_cur_block_universe);
                        m_bv_block_itr = list.begin();
                        m_bv_block_end = list.end();
                    }
                    if (m_cur_block_type == uef_blocktype::EF) {
                        auto list = eliasfano_list<true,true>::materialize(*m_iss,m_blockstart[0],items_in_block,m_cur_block_universe);
                        m_ef_block_itr = list.begin();
                        m_ef_block_end = list.end();
                    }
                    access_current_elem(); // access the first item
                }
            }
        }
    public:
        size_type offset() const
        {
            return m_cur_offset;
        }
        size_type size() const
        {
            return m_size;
        }
        size_type remaining() const
        {
            return m_size - m_cur_offset;
        }
        uint64_t operator*() const
        {
            if (m_num_blocks == 1) return
                    *(m_top_itr+m_cur_offset);
            if (m_last_accessed_offset != m_cur_offset) {
                access_current_elem();
                m_last_accessed_offset = m_cur_offset;
            }
            return m_cur_elem;
        }
        bool operator ==(const uniform_ef_iterator& b) const
        {
            return m_cur_offset == b.m_cur_offset;
        }
        bool operator !=(const uniform_ef_iterator& b) const
        {
            return m_cur_offset != b.m_cur_offset;
        }
        uniform_ef_iterator operator++(int)
        {
            uniform_ef_iterator tmp(*this);
            m_cur_offset++;
            return tmp;
        }
        uniform_ef_iterator& operator++()
        {
            m_cur_offset++;
            return *this;
        }
        uniform_ef_iterator& operator+=(size_type i)
        {
            if (i == 0) return *this;
            m_cur_offset += i;
            return *this;
        }
        uniform_ef_iterator operator+(size_type i)
        {
            uniform_ef_iterator tmp(*this);
            tmp += i;
            return tmp;
        }
        template<class t_itr>
        auto operator-(const t_itr& b) const -> difference_type
        {
            return (difference_type)offset() - (difference_type)b.offset();
        }
        bool skip(uint64_t pos)
        {
            if (m_num_blocks == 1) return m_top_itr.skip(pos);
            auto cur_block = m_cur_offset/t_block_size;
            if (pos > *m_top_itr) {
                // skip to correct block
                m_top_itr.skip(pos);
                if (m_top_itr == m_top_end) {
                    m_cur_offset = m_size;
                    return false;
                }
                auto new_block = m_top_itr.offset();
                if (new_block != cur_block) { // we are in a new block!
                    auto prev = m_top_itr; --prev;
                    m_prev_top = *prev;
                    m_cur_block_value_offset = m_prev_top + 1;
                    m_cur_block_universe = *m_top_itr - m_prev_top - 1;
                    auto block_start_offset = new_block*t_block_size;
                    auto items_in_block = (m_size - block_start_offset) < t_block_size ? m_size - block_start_offset : t_block_size;
                    m_cur_block_type = determine_block_type(items_in_block,m_cur_block_universe);
                    if (m_cur_block_type == uef_blocktype::BV) {
                        auto list = bitvector_list<true>::materialize(*m_iss,m_blockstart[new_block],items_in_block,m_cur_block_universe);
                        m_bv_block_itr = list.begin();
                        m_bv_block_end = list.end();
                    }
                    if (m_cur_block_type == uef_blocktype::EF) {
                        auto list = eliasfano_list<true,true>::materialize(*m_iss,m_blockstart[new_block],items_in_block,m_cur_block_universe);
                        m_ef_block_itr = list.begin();
                        m_ef_block_end = list.end();
                    }
                }
            }
            // look inside block. invariant pos <= TOP[block]
            auto rel_pos = pos - m_cur_block_value_offset;
            if (m_cur_block_type == uef_blocktype::BV) {
                bool found = m_bv_block_itr.skip(rel_pos);
                m_cur_offset = m_top_itr.offset()*t_block_size + m_bv_block_itr.offset();
                m_cur_elem = m_cur_block_value_offset + *m_bv_block_itr;
                m_last_accessed_offset = m_cur_offset;
                return found;
            }
            if (m_cur_block_type == uef_blocktype::EF) {
                bool found = m_ef_block_itr.skip(rel_pos);
                m_cur_offset = m_top_itr.offset()*t_block_size + m_ef_block_itr.offset();
                m_cur_elem = m_cur_block_value_offset + *m_ef_block_itr;
                m_last_accessed_offset = m_cur_offset;
                return found;
            }
            m_cur_offset += rel_pos; // must be found in a full block
            m_cur_elem = m_cur_block_value_offset + rel_pos;
            m_last_accessed_offset = m_cur_offset;
            return true;
        }
    private:
        void access_current_elem() const
        {
            auto block = m_cur_offset/t_block_size;
            if (block != m_top_itr.offset()) { // move to correct block
                auto diff = block - m_top_itr.offset();
                m_top_itr += diff-1;
                m_prev_top = *m_top_itr;
                m_top_itr++;
                m_cur_block_value_offset = m_prev_top + 1;
                m_cur_block_universe = *m_top_itr - m_prev_top - 1;
                auto block_start_offset = block*t_block_size;
                auto items_in_block = (m_size - block_start_offset) < t_block_size ? m_size - block_start_offset : t_block_size;
                m_cur_block_type = determine_block_type(items_in_block,m_cur_block_universe);
                if (m_cur_block_type == uef_blocktype::BV) {
                    auto list = bitvector_list<true>::materialize(*m_iss,m_blockstart[block],items_in_block,m_cur_block_universe);
                    m_bv_block_itr = list.begin();
                    m_bv_block_end = list.end();
                }
                if (m_cur_block_type == uef_blocktype::EF) {
                    auto list = eliasfano_list<true,true>::materialize(*m_iss,m_blockstart[block],items_in_block,m_cur_block_universe);
                    m_ef_block_itr = list.begin();
                    m_ef_block_end = list.end();
                }
            }

            auto in_block_offset = m_cur_offset%t_block_size;
            switch (m_cur_block_type) {
                case uef_blocktype::BV:
                    if (in_block_offset != m_bv_block_itr.offset())
                        m_bv_block_itr += (in_block_offset - m_bv_block_itr.offset());
                    m_cur_elem = m_cur_block_value_offset + *m_bv_block_itr;
                    break;
                case uef_blocktype::EF:
                    if (in_block_offset != m_ef_block_itr.offset())
                        m_ef_block_itr += (in_block_offset - m_ef_block_itr.offset());
                    m_cur_elem = m_cur_block_value_offset + *m_ef_block_itr;
                    break;
                case uef_blocktype::FULL:
                    m_cur_elem = m_cur_block_value_offset + in_block_offset;
                    break;
            }
        }
};

template<uint16_t t_block_size = 128>
struct uniform_eliasfano_list {
    static_assert(t_block_size % 32 == 0,"blocksize must be multiple of 32.");
    using size_type = sdsl::int_vector<>::size_type;
    using iterator_type = uniform_ef_iterator<t_block_size>;
    using list_type = list_dummy<iterator_type>;
    using top_list_type = eliasfano_skip_list<64,true,false>;

    static void
    compress_block(bit_ostream& os,const sdsl::int_vector<64>& data,size_t m,size_t n)
    {
        auto block_type = determine_block_type(m,n);
        switch (block_type) {
            case uef_blocktype::BV:
                bitvector_list<true>::create(os,data.begin(),data.begin()+m,n);
                break;
            case uef_blocktype::EF:
                eliasfano_list<true,true>::create(os,data.begin(),data.begin()+m,m,n);
                break;
            case uef_blocktype::FULL:
                break;
        }
    }

    template<class t_itr>
    static size_type create(bit_ostream& os,t_itr begin,t_itr end)
    {
        size_type data_offset = os.tellp();

        uint64_t num_items = std::distance(begin,end);
        uint64_t num_full_blocks = num_items/t_block_size;
        uint64_t num_blocks = num_full_blocks;
        bool has_leftover_block = false;
        if (num_items%t_block_size!=0) {
            has_leftover_block = true;
            num_blocks++;
        }

        // (0) write list meta data
        os.encode<coder::elias_gamma>(num_items);

        if (num_blocks == 1) {
            top_list_type::create(os,begin,end);
        } else {
            // (1) block start data
            os.expand_if_needed((num_blocks+1)*64);
            os.align64();
            auto block_start_offset = os.tellp()>>6;
            os.skip(num_blocks*64);

            // (2) top level
            sdsl::int_vector<64> top_lvl(num_blocks);
            auto itr = begin+t_block_size-1ULL;
            size_t j=0;
            for (size_t i=t_block_size-1; i<num_items; i+=t_block_size) {
                top_lvl[j++] = *itr;
                itr+= t_block_size;
            }
            if (has_leftover_block) {
                top_lvl[j] = *(end-1);
            }
            top_list_type::create(os,top_lvl.begin(),top_lvl.end());

            // (3) bottom level
            sdsl::int_vector<64> tmp_data(t_block_size);
            sdsl::int_vector<64> tmp_data2(t_block_size);
            itr = begin;
            size_t value_offset = 0;
            for (size_t i=0; i<num_full_blocks; i++) {
                uint64_t* block_start = os.data()+block_start_offset;
                block_start[i] = os.tellp();
                // (3a) compute block data
                value_offset = 0;
                if (i!=0) value_offset = top_lvl[i-1] + 1ULL;
                for (size_t j=0; j<t_block_size; j++) {
                    tmp_data[j] = *itr - value_offset;
                    tmp_data2[j] = *itr;
                    ++itr;
                }
                // (3b) compress block
                size_t block_universe = top_lvl[i];
                if (i!=0) block_universe = block_universe - top_lvl[i-1] - 1;
                compress_block(os,tmp_data,t_block_size,block_universe);
            }
            // (4) last block
            if (has_leftover_block) { // encode last block
                size_t left = num_items%t_block_size;
                uint64_t* block_start = os.data()+block_start_offset;
                block_start[num_blocks-1] = os.tellp();
                value_offset = 0;
                if (num_blocks!=1) value_offset = top_lvl[num_blocks-2] + 1;
                for (size_t j=0; j<left; j++) {
                    tmp_data[j] = *itr - value_offset;
                    ++itr;
                }
                size_t block_universe = top_lvl[num_blocks-1];
                if (num_blocks!=0) block_universe = block_universe - top_lvl[num_blocks-2] -1;
                compress_block(os,tmp_data,left,block_universe);
            }
        }

        return data_offset;
    }

    static list_dummy<iterator_type> materialize(const bit_istream& is,size_t start_offset)
    {
        return list_dummy<iterator_type>(iterator_type(is,start_offset,false),iterator_type(is,start_offset,true));
    }
};

