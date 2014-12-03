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

template<uint16_t t_block_size,bool t_sorted>
class optpfor_iterator : public std::iterator<std::random_access_iterator_tag,uint64_t,std::ptrdiff_t>
{
    public:
        using size_type = sdsl::int_vector<>::size_type;
        using comp_codec = FastPForLib::OPTPFor<t_block_size/32,FastPForLib::Simple16<false>>;
    private:
        uint64_t m_size;
        uint64_t m_min_offset = 0;
        mutable uint32_t m_tmp_data[t_block_size];
        const uint32_t* m_block_start;
        const uint32_t* m_block_max;
        const uint32_t* m_data;
        uint64_t m_num_blocks;

    private:
        mutable comp_codec c;
        uint64_t m_cur_block = 0;
        uint64_t m_cur_offset = 0;
        mutable uint64_t m_last_accessed_block = 0;
        bool m_last_block_vbyte = false;
    public:
        optpfor_iterator(const bit_istream& is,size_t start_offset,bool end)
        {
            is.seek(start_offset);
            m_size = is.decode<coder::elias_gamma>();
            if (!t_sorted) {
                m_min_offset = is.decode<coder::elias_gamma>();
            }

            if (m_size <= t_block_size) {
                if (!end) {
                    if (t_sorted) {
                        is.decode<coder::delta<coder::vbyte>>(std::begin(m_tmp_data),m_size);
                    } else {
                        is.decode<coder::vbyte>(std::begin(m_tmp_data),m_size);
                    }
                }
            } else {
                is.align64();
                m_num_blocks = m_size / t_block_size;
                if (m_size % t_block_size != 0) {
                    m_num_blocks++;
                    m_last_block_vbyte = true;
                }
                m_block_start = reinterpret_cast<const uint32_t*>(is.cur_data());
                m_block_max = m_block_start + m_num_blocks;
                if (t_sorted) {
                    is.skip(m_num_blocks*32*2);
                } else {
                    is.skip(m_num_blocks*32);
                }
                is.align64();
                m_data = reinterpret_cast<const uint32_t*>(is.cur_data());
                if (!end) decode_current_block();
            }
            if (end) m_cur_offset = m_size;
            m_last_accessed_block = 0;
        }
        optpfor_iterator() = delete;
        optpfor_iterator(const optpfor_iterator& pi) = default;
        optpfor_iterator(optpfor_iterator&& pi) = default;
        optpfor_iterator& operator=(const optpfor_iterator& pi) = default;
        optpfor_iterator& operator=(optpfor_iterator&& pi) = default;
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
            if (m_cur_block != m_last_accessed_block) {
                decode_current_block();
            }
            return m_tmp_data[m_cur_offset%t_block_size] + m_min_offset;
        }
        bool operator ==(const optpfor_iterator& b) const
        {
            return m_cur_offset == b.m_cur_offset;
        }
        bool operator !=(const optpfor_iterator& b) const
        {
            return m_cur_offset != b.m_cur_offset;
        }
        optpfor_iterator operator++(int)
        {
            optpfor_iterator tmp(*this);
            m_cur_offset++;
            m_cur_block = m_cur_offset/t_block_size;
            return tmp;
        }
        optpfor_iterator& operator++()
        {
            m_cur_offset++;
            m_cur_block = m_cur_offset/t_block_size;
            return *this;
        }
        optpfor_iterator& operator+=(size_type i)
        {
            m_cur_offset+=i;
            m_cur_block = m_cur_offset/t_block_size;
            return *this;
        }
        optpfor_iterator operator+(size_type i)
        {
            optpfor_iterator tmp(*this);
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
            static_assert(t_sorted == true,"skipping only works in sorted lists.");
            auto in_block_offset = m_cur_offset%t_block_size;
            if (m_tmp_data[in_block_offset]==pos) return true;
            if (m_size > t_block_size && m_block_max[m_cur_block] < pos) { // more than one block?
                // find correct block
                bool found = false;
                for (size_t i=m_cur_block+1; i<m_num_blocks; i++) {
                    if (m_block_max[i] >= pos) {
                        m_cur_block = i;
                        in_block_offset = 0;
                        found = true;
                        if (m_block_max[i] == pos) {
                            m_cur_block = i;
                            size_t block_size = (m_cur_block == m_num_blocks-1)  ? m_size % t_block_size : t_block_size;
                            in_block_offset = block_size-1;
                            m_cur_offset = m_cur_block*t_block_size + in_block_offset;
                            return true;
                        }
                        decode_current_block();
                        break;
                    }
                }
                // not found
                if (!found) {
                    m_cur_offset = m_size;
                    return false;
                }
            }
            // search in block
            size_t block_size = (m_cur_block == m_num_blocks-1)  ? m_size % t_block_size : t_block_size;
            for (size_t i=in_block_offset; i<block_size; i++) {
                if (m_tmp_data[i] >= pos) {
                    m_cur_offset = m_cur_block*t_block_size + i;
                    if (m_tmp_data[i] == pos) return true;
                    return false;
                }
            }
            // nothing found in the expected block. move to next block
            if (m_cur_block == m_num_blocks-1) {
                m_cur_offset = m_size; // finished
            } else {
                m_cur_block++;
                m_cur_offset = m_cur_block*t_block_size;
            }
            return false;
        }
    private:
        void decode_current_block() const
        {
            const uint32_t* block_data = m_data + m_block_start[m_cur_block];
            if (m_cur_block == m_num_blocks-1 && m_last_block_vbyte) { // last block?
                size_t block_size = m_size%t_block_size;
                utils::vbyte_coder::decode(block_data,block_size,m_tmp_data);
            } else {
                size_t decoded_elems = 0;
                c.decodeBlock(block_data,m_tmp_data,decoded_elems);
            }
            if (t_sorted) {
                if (m_cur_block!=0) m_tmp_data[0] += m_block_max[m_cur_block-1];
                for (size_t i=1; i<t_block_size; i++) m_tmp_data[i] += m_tmp_data[i-1];
            }
            m_last_accessed_block = m_cur_block;
        }
};

template<uint16_t t_block_size = 128,bool t_sorted = true>
struct optpfor_list {
    static_assert(t_block_size % 32 == 0,"blocksize must be multiple of 32.");
    using size_type = sdsl::int_vector<>::size_type;
    using comp_codec = FastPForLib::OPTPFor<t_block_size/32,FastPForLib::Simple16<false>>;
    using iterator_type = optpfor_iterator<t_block_size,t_sorted>;
    using list_type = list_dummy<iterator_type>;
    template<class t_itr>
    static size_type create(bit_ostream& os,t_itr begin,t_itr end)
    {
        size_type data_offset = os.tellp();

        // compute properties
        uint64_t size = std::distance(begin,end);

        // write size
        os.encode<coder::elias_gamma>(size);

        size_t min = 0;
        if (!t_sorted) {
            auto min_elem = std::min_element(begin,end);
            min = *min_elem;
            os.encode<coder::elias_gamma>(min);
        }

        // one block special case
        if (size <= t_block_size) {
            if (t_sorted) {
                os.encode<coder::delta<coder::vbyte>>(begin,end);
            } else {
                auto tmp = begin;
                while (tmp != end) {
                    os.encode<coder::vbyte>(*tmp - min);
                    ++tmp;
                }
            }
        } else {
            uint64_t num_blocks = size / t_block_size;
            if (size % t_block_size != 0) num_blocks++;

            // reserve meta data first
            os.expand_if_needed((2*num_blocks*32)+(num_blocks*t_block_size*32)); // overestimate space required
            os.align64();
            uint32_t* meta_data = reinterpret_cast<uint32_t*>(os.cur_data());
            if (t_sorted) { // need more space for skip data
                os.skip(num_blocks*32*2);
            } else {
                os.skip(num_blocks*32);
            }
            os.align64();

            // encode
            comp_codec c;
            uint32_t* pos_data = reinterpret_cast<uint32_t*>(os.cur_data());
            size_t block_start = 0;
            uint32_t tmp_data[t_block_size+1];
            auto itr = begin;
            tmp_data[0] = 0;
            for (size_t i=0; i<num_blocks; i++) {
                if (i!=0) tmp_data[0] = meta_data[num_blocks+i-1];
                meta_data[i] = block_start;
                size_t n = std::min((size_t)t_block_size,size-(t_block_size*i));
                std::copy(itr,itr+n,std::begin(tmp_data)+1);
                if (t_sorted) {
                    meta_data[num_blocks+i] = tmp_data[n];
                    FastPForLib::Delta::fastDelta(tmp_data,n+1);
                } else {
                    for (size_t j=0; j<=n; j++) tmp_data[j] -= min;
                }
                size_t encoded_id_size = 0;
                if (n == t_block_size) {
                    c.encodeBlock(tmp_data+1,pos_data+block_start,encoded_id_size);
                } else {
                    utils::vbyte_coder::encode(tmp_data+1,n,pos_data+block_start,encoded_id_size);
                }
                block_start += encoded_id_size;
                itr += n;
            }
            os.skip(block_start*32);
        }
        return data_offset;
    }

    static list_dummy<iterator_type> materialize(const bit_istream& is,size_t start_offset)
    {
        return list_dummy<iterator_type>(iterator_type(is,start_offset,false),iterator_type(is,start_offset,true));
    }
};

