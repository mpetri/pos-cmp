#pragma once

#include <iterator>

#include "bit_streams.hpp"
#include "bit_coders.hpp"
#include "bit_magic.hpp"

#include "list_basics.hpp"


template<bool t_compact>
class bv_iterator : public std::iterator<std::random_access_iterator_tag,uint64_t,std::ptrdiff_t>
{
    public:
        using size_type = sdsl::int_vector<>::size_type;
    private:
        uint64_t m_size;
        uint64_t m_universe;
        const uint64_t* m_data;
    private:
        mutable value_type m_cur_elem = 0;
        size_type m_bit_start_offset = 0;
        mutable size_type m_bit_offset = 0;
        size_type m_cur_offset = 0;
    public:
        bv_iterator(const bit_istream& is,size_t start_offset,bool end)
        {
            static_assert(t_compact == false,"Can't use this constructor with compact ef representation.");
            m_data = is.data();
            is.seek(start_offset);
            m_size = is.decode<coder::elias_gamma>();
            m_universe = is.decode<coder::elias_gamma>();
            m_bit_offset = is.tellg();
            m_bit_start_offset = m_bit_offset;
            m_cur_offset = end ? m_size : 0;
            const auto data_ptr = m_data + (m_bit_offset>>6);
            const auto in_word_offset = m_bit_offset&0x3F;
            if (m_cur_offset == 0 && sdsl::bits::read_int(data_ptr,in_word_offset,1) == 0) {
                m_bit_offset = sdsl::bits::next(m_data,m_bit_offset); // select first one
            }
        }
        bv_iterator(const bit_istream& is,size_t start_offset,bool end,size_type size,size_type universe)
            : m_size(size), m_universe(universe)
        {
            m_data = is.data();
            is.seek(start_offset);
            m_cur_offset = end ? m_size : 0;
            m_bit_offset = is.tellg();
            m_bit_start_offset = m_bit_offset;
            const auto data_ptr = m_data + (m_bit_offset>>6);
            const auto in_word_offset = m_bit_offset&0x3F;
            if (m_cur_offset == 0 && sdsl::bits::read_int(data_ptr,in_word_offset,1) == 0) {
                m_bit_offset = sdsl::bits::next(m_data,m_bit_offset); // select first one
            }
        }
        bv_iterator() = default;
        bv_iterator(const bv_iterator& pi) = default;
        bv_iterator(bv_iterator&& pi) = default;
        bv_iterator& operator=(const bv_iterator& pi) = default;
        bv_iterator& operator=(bv_iterator&& pi) = default;
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
            return m_bit_offset - m_bit_start_offset;
        }
        bool operator ==(const bv_iterator& b) const
        {
            return m_cur_offset == b.m_cur_offset;
        }
        bool operator !=(const bv_iterator& b) const
        {
            return m_cur_offset != b.m_cur_offset;
        }
        bv_iterator operator++(int)
        {
            bv_iterator tmp(*this);
            m_bit_offset = sdsl::bits::next(m_data,m_bit_offset+1);
            m_cur_offset++;
            return tmp;
        }
        bv_iterator& operator++()
        {
            m_bit_offset = sdsl::bits::next(m_data,m_bit_offset+1);
            m_cur_offset++;
            return *this;
        }
        bv_iterator& operator+=(size_type i)
        {
            if (i == 0) return *this;
            m_bit_offset = bit_magic::next_Xth_one(m_data,m_bit_offset,i);
            m_cur_offset += i;
            return *this;
        }
        bv_iterator operator+(size_type i)
        {
            bv_iterator tmp(*this);
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
            size_type cur_pos = m_bit_offset - m_bit_start_offset;
            if (pos < cur_pos) return false;
            if (pos > m_universe) {
                m_cur_offset = m_size;
                return false;
            }
            m_bit_offset = m_bit_start_offset + pos;
            const auto data_ptr = m_data + (m_bit_offset>>6);
            const auto in_word_offset = m_bit_offset&0x3F;
            if (sdsl::bits::read_int(data_ptr,in_word_offset,1) == 0) {
                m_bit_offset = sdsl::bits::next(m_data,m_bit_offset); // select next one
                if (m_bit_offset - m_bit_start_offset > m_universe) {
                    m_cur_offset = m_size;
                }
                return false;
            }
            if (m_bit_offset - m_bit_start_offset > m_universe) {
                m_cur_offset = m_size;
                return false;
            }
            return true;
        }
};

template<bool t_compact = false>
struct bitvector_list {
    using size_type = sdsl::int_vector<>::size_type;
    using iterator_type = bv_iterator<t_compact>;
    using list_type = list_dummy<iterator_type>;

    template<class t_itr>
    static size_type create(bit_ostream& os,t_itr begin,t_itr end,size_type u = 0)
    {
        size_type data_offset = os.tellp();

        // compute properties
        uint64_t m = std::distance(begin,end);
        if (u==0) u = *(end-1);

        if (!t_compact) {
            // write size
            os.encode<coder::elias_gamma>(m);
            os.encode<coder::elias_gamma>(u);
        }

        // write the bits
        os.expand_if_needed(u+1);
        auto itr = begin;
        auto last = *itr;
        os.put_unary_no_size_check(last);
        ++itr;
        while (itr != end) {
            auto position = *itr;
            auto value = position - last;
            last = position;
            os.put_unary_no_size_check(value-1);
            ++itr;
        }
        os.put(1); // terminating 1 bit
        return data_offset;
    }

    static list_dummy<iterator_type> materialize(const bit_istream& is,size_t start_offset)
    {
        return list_dummy<iterator_type>(iterator_type(is,start_offset,false),iterator_type(is,start_offset,true));
    }

    static list_dummy<iterator_type> materialize(const bit_istream& is,size_t start_offset,size_type m,size_type u)
    {
        return list_dummy<iterator_type>(iterator_type(is,start_offset,false,m,u),iterator_type(is,start_offset,true,m,u));
    }

    static size_type estimate_size(size_type ,size_type u)
    {
        return u+1;
    }
};

