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

template<uint16_t t_block_size>
class uniform_eliasfano_iterator : public std::iterator<std::random_access_iterator_tag,uint64_t,std::ptrdiff_t>
{
    public:
        using size_type = sdsl::int_vector<>::size_type;
    private:
        uint64_t m_size;
        uint64_t m_min_offset = 0;
        mutable uint32_t m_tmp_data[t_block_size];
        const uint32_t* m_block_start;
        const uint32_t* m_block_max;
        const uint32_t* m_data;
        uint64_t m_num_blocks;
    private:
        uint64_t m_cur_block = 0;
        uint64_t m_cur_offset = 0;
        mutable uint64_t m_last_accessed_block = 0;
    public:
        uniform_eliasfano_iterator(const bit_istream& is,size_t start_offset,bool end)
        {

        }
        uniform_eliasfano_iterator() = delete;
        uniform_eliasfano_iterator(const uniform_eliasfano_iterator& pi) = default;
        uniform_eliasfano_iterator(uniform_eliasfano_iterator&& pi) = default;
        uniform_eliasfano_iterator& operator=(const uniform_eliasfano_iterator& pi) = default;
        uniform_eliasfano_iterator& operator=(uniform_eliasfano_iterator&& pi) = default;
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
        }
        bool operator ==(const uniform_eliasfano_iterator& b) const
        {
        }
        bool operator !=(const uniform_eliasfano_iterator& b) const
        {
        }
        uniform_eliasfano_iterator operator++(int)
        {
            uniform_eliasfano_iterator tmp(*this);
            m_cur_offset++;
            m_cur_block = m_cur_offset/t_block_size;
            return tmp;
        }
        uniform_eliasfano_iterator& operator++()
        {
            m_cur_offset++;
            m_cur_block = m_cur_offset/t_block_size;
            return *this;
        }
        uniform_eliasfano_iterator& operator+=(size_type i)
        {
            m_cur_offset+=i;
            m_cur_block = m_cur_offset/t_block_size;
            return *this;
        }
        uniform_eliasfano_iterator operator+(size_type i)
        {
            uniform_eliasfano_iterator tmp(*this);
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

        }
};

template<uint16_t t_block_size = 128>
struct uniform_eliasfano_list {
    static_assert(t_block_size % 32 == 0,"blocksize must be multiple of 32.");
    using size_type = sdsl::int_vector<>::size_type;
    using iterator_type = uniform_eliasfano_iterator<t_block_size>;
    using list_type = list_dummy<iterator_type>;
    template<class t_itr>
    static size_type create(bit_ostream& os,t_itr begin,t_itr end)
    {
        size_type data_offset = os.tellp();

        uint64_t num_items = std::distance(begin,end);
        uint64_t num_blocks = num_items/t_block_size;
        if (num_items % t_block_size != 0) num_blocks++;

        // (0) write list meta data
        os.encode<coder::elias_gamma>(num_items);

        // (1) block start data
        os.align64();
        uint32_t* block_start = reinterpret_cast<uint32_t*>(os.cur_data());
        os.skip(num_blocks*32);
        os.align64();

        // (2) top level
        skip_iterator<t_itr> skip_begin(begin,end,t_block_size);
        skip_iterator<t_itr> skip_end(end,end,t_block_size);
        eliasfano_list<true,false>::create(os,skip_begin,skip_end);

        // (3) bottom level
        size_type block_offset = os.tellp();
        size_type cur_block = 0;
        uint64_t tmp_data[t_block_size];
        for (size_t i=0; i<num_items; i+=t_block_size) {
            block_start[cur_block] = os.tellp() - block_offset;
            // (3a) compute block data
            size_t data_offset = *begin;
            for (size_t j=0; j<t_block_size; j++) {
                tmp_data[j] = *begin - data_offset;
                ++begin;
            }
            // (3b) compress block
            size_t block_universe = tmp_data[t_block_size-1]+1;
            auto ef_bits = eliasfano_list<true,true>::estimate_size(t_block_size,block_universe);
            auto bv_bits = block_universe;
            if (block_universe != t_block_size) {
                if (bv_bits < ef_bits) {

                } else {
                    eliasfano_list<true,true>::create(os,tmp_data.begin(),tmp_data.end());
                }
            }
        }
        // (4) last block

        return data_offset;
    }

    static list_dummy<iterator_type> materialize(const bit_istream& is,size_t start_offset)
    {
        return list_dummy<iterator_type>(iterator_type(is,start_offset,false),iterator_type(is,start_offset,true));
    }
};

