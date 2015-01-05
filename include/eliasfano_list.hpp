#pragma once

#include <iterator>

#include "bit_streams.hpp"
#include "bit_coders.hpp"
#include "bit_magic.hpp"

#include "list_basics.hpp"

template<bool t_sorted,bool t_compact>
class ef_iterator : public std::iterator<std::random_access_iterator_tag,uint64_t,std::ptrdiff_t>
{
    public:
        using size_type = sdsl::int_vector<>::size_type;
    private:
        uint64_t m_size = 0;
        uint64_t m_universe = 0;
        uint8_t m_width_low = 0;
        uint64_t m_low_offset = 0;
        uint64_t m_high_offset = 0;
        const uint64_t* m_data;
    private:
        mutable value_type m_cur_elem = 0;
        mutable size_type m_last_accessed_offset = std::numeric_limits<uint64_t>::max();
        size_type m_cur_high_offset = 0;
        size_type m_cur_offset = 0;
        mutable value_type m_prev_elem = 0;
    public:
        ef_iterator(const bit_istream& is,size_t start_offset,bool end)
        {
            static_assert(t_compact == false,"Can't use this constructor with compact ef representation.");
            m_data = is.data();
            is.seek(start_offset);
            m_size = is.decode<coder::elias_gamma>();
            m_universe = is.decode<coder::elias_gamma>();
            uint8_t logm = sdsl::bits::hi(m_size)+1;
            uint8_t logu = sdsl::bits::hi(m_universe)+1;
            if (logu < logm) {
                m_width_low = 1;
            } else {
                if (logm == logu) logm--;
                m_width_low = logu - logm;
            }
            m_low_offset = is.tellg();
            m_high_offset = m_low_offset + m_size*m_width_low;
            m_cur_offset = end ? m_size : 0;
            m_cur_high_offset = 0;
            if (m_cur_offset == 0) {
                if (high(0) != 1) {
                    m_cur_high_offset = sdsl::bits::next(m_data,m_high_offset+1) - m_high_offset;
                }
            }
            size_t cur_bucket = m_cur_high_offset - m_cur_offset;
            m_cur_elem = (cur_bucket << m_width_low) | low(m_cur_offset);
            m_last_accessed_offset = m_cur_offset;
        }
        ef_iterator(const bit_istream& is,size_t start_offset,bool end,size_type size,size_type universe)
            : m_size(size) , m_universe(universe)
        {
            uint8_t logm = sdsl::bits::hi(m_size)+1;
            uint8_t logu = sdsl::bits::hi(m_universe)+1;
            if (logu < logm) {
                m_width_low = 1;
            } else {
                if (logm == logu) logm--;
                m_width_low = logu - logm;
            }

            m_data = is.data();
            is.seek(start_offset);
            m_low_offset = is.tellg();
            m_high_offset = m_low_offset + m_size*m_width_low;
            m_cur_offset = end ? m_size : 0;
            m_cur_high_offset = 0;
            if (m_cur_offset == 0) {
                if (high(0) != 1) {
                    m_cur_high_offset = sdsl::bits::next(m_data,m_high_offset+1) - m_high_offset;
                }
            }
            size_t cur_bucket = m_cur_high_offset - m_cur_offset;
            m_cur_elem = (cur_bucket << m_width_low) | low(m_cur_offset);
            m_last_accessed_offset = m_cur_offset;
        }
        ef_iterator() = default;
        ef_iterator(const ef_iterator& pi) = default;
        ef_iterator(ef_iterator&& pi) = default;
        ef_iterator& operator=(const ef_iterator& pi) = default;
        ef_iterator& operator=(ef_iterator&& pi) = default;
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
            if (!t_sorted) {
                if (m_last_accessed_offset != m_cur_offset) {
                    if (m_cur_offset == 0 || m_last_accessed_offset+1 == m_cur_offset) {
                        m_prev_elem = m_cur_elem;
                    } else {
                        size_type poffset = m_high_offset + m_cur_high_offset;
                        auto prev_high_offset = sdsl::bits::prev(m_data,poffset-1) - m_high_offset;
                        size_t prev_bucket = prev_high_offset - (m_cur_offset - 1);
                        m_prev_elem = (prev_bucket << m_width_low) | low(m_cur_offset-1);
                        size_t cur_bucket = m_cur_high_offset - m_cur_offset;
                        m_cur_elem = (cur_bucket << m_width_low) | low(m_cur_offset);
                    }
                    size_t cur_bucket = m_cur_high_offset - m_cur_offset;
                    m_cur_elem = (cur_bucket << m_width_low) | low(m_cur_offset);
                    m_last_accessed_offset = m_cur_offset;
                }
                return m_cur_elem - m_prev_elem;
            } else {
                if (m_last_accessed_offset != m_cur_offset) {
                    size_t cur_bucket = m_cur_high_offset - m_cur_offset;
                    m_cur_elem = (cur_bucket << m_width_low) | low(m_cur_offset);
                    m_last_accessed_offset = m_cur_offset;
                }
                return m_cur_elem;
            }
        }
        bool operator ==(const ef_iterator& b) const
        {
            return m_cur_offset == b.m_cur_offset;
        }
        bool operator !=(const ef_iterator& b) const
        {
            return m_cur_offset != b.m_cur_offset;
        }
        ef_iterator operator++(int)
        {
            ef_iterator tmp(*this);
            size_type offset = m_high_offset + m_cur_high_offset;
            m_cur_high_offset = sdsl::bits::next(m_data,offset+1) - m_high_offset;
            m_cur_offset++;
            return tmp;
        }
        ef_iterator& operator++()
        {
            size_type offset = m_high_offset + m_cur_high_offset;
            m_cur_high_offset = sdsl::bits::next(m_data,offset+1) - m_high_offset;
            m_cur_offset++;
            return *this;
        }
        ef_iterator& operator+=(size_type i)
        {
            if (i == 0) return *this;
            size_type offset = m_high_offset + m_cur_high_offset;
            m_cur_high_offset = bit_magic::next_Xth_one(m_data,offset,i) - m_high_offset;
            m_cur_offset += i;
            return *this;
        }
        ef_iterator operator+(size_type i)
        {
            ef_iterator tmp(*this);
            tmp += i;
            return tmp;
        }
        ef_iterator& operator--()
        {
            size_type offset = m_high_offset + m_cur_high_offset;
            m_cur_high_offset = sdsl::bits::prev(m_data,offset-1) - m_high_offset;
            m_cur_offset--;
            return *this;
        }
        template<class t_itr>
        auto operator-(const t_itr& b) const -> difference_type
        {
            return (difference_type)offset() - (difference_type)b.offset();
        }
        bool skip(uint64_t pos)
        {
            static_assert(t_sorted == true,"skipping only works in sorted lists.");
            if (m_cur_elem == pos) return true;
            if (m_cur_elem > pos) return false;
            if (m_universe < pos) {
                m_cur_offset = m_size;
                return false;
            }
            uint64_t high_bucket = pos >> m_width_low;
            uint64_t cur_bucket = m_cur_high_offset - m_cur_offset;
            if (cur_bucket > high_bucket) return false;
            if (high_bucket != cur_bucket) {
                // skip to the one after high_bucket 0's
                auto skip_zeros = high_bucket - cur_bucket;
                auto zero_pos = bit_magic::next_Xth_zero(m_data,m_high_offset+m_cur_high_offset,skip_zeros) - m_high_offset;
                m_cur_offset = m_cur_offset + (zero_pos - m_cur_high_offset) - skip_zeros + 1;
                m_cur_high_offset = sdsl::bits::next(m_data,m_high_offset+zero_pos) - m_high_offset;
                if (m_cur_high_offset != zero_pos+1) {
                    // if the bucket we hit doesn't exactly match we don't have to search inside the bucket
                    cur_bucket = m_cur_high_offset - m_cur_offset;
                    m_cur_elem = (cur_bucket << m_width_low) | low(m_cur_offset);
                    m_last_accessed_offset = m_cur_offset;
                    return false;
                }
            }
            // scan the lower part of the current bucket
            auto next_zero_pos = bit_magic::next0(m_data,m_high_offset+m_cur_high_offset) - m_high_offset;
            auto remaining_bucket_size = next_zero_pos - m_cur_high_offset;
            size_t i = 0;
            uint64_t low_num = pos&((1ULL<<m_width_low)-1);
            for (i=m_cur_offset; i<m_cur_offset+remaining_bucket_size; i++) {
                auto cur_low = low(i);
                if (cur_low >= low_num) {
                    // found pos or found a larger with the bucket!
                    auto ones_skipped = i-m_cur_offset;
                    m_cur_high_offset = m_cur_high_offset + ones_skipped;
                    m_cur_offset += ones_skipped;
                    m_cur_elem = (high_bucket << m_width_low) | cur_low;
                    m_last_accessed_offset = m_cur_offset;
                    if (m_cur_elem == pos) return true;
                    return false;
                }
            }
            // nothing found in the bucket we were interested in. move on to the next larger bucket
            m_cur_high_offset = sdsl::bits::next(m_data,m_high_offset+next_zero_pos) - m_high_offset;
            m_cur_offset += remaining_bucket_size;
            cur_bucket = m_cur_high_offset - m_cur_offset;
            m_cur_elem = (cur_bucket << m_width_low) | low(i);
            m_last_accessed_offset = m_cur_offset;
            return false;
        }
    private:
        inline value_type low(size_type i) const
        {
            const auto off = m_low_offset + i*m_width_low;
            const auto data_ptr = m_data + (off>>6);
            const auto in_word_offset = off&0x3F;
            return sdsl::bits::read_int(data_ptr,in_word_offset,m_width_low);
        }
        inline value_type high(size_type i) const
        {
            const auto off = m_high_offset + i;
            const auto data_ptr = m_data + (off>>6);
            const auto in_word_offset = off&0x3F;
            return sdsl::bits::read_int(data_ptr,in_word_offset,1);
        }
};

template<bool t_sorted = true,bool t_compact = false>
struct eliasfano_list {
    using size_type = sdsl::int_vector<>::size_type;
    using iterator_type = ef_iterator<t_sorted,t_compact>;
    using list_type = list_dummy<iterator_type>;

    template<class t_itr>
    static size_type create(bit_ostream& os,t_itr begin,t_itr end,size_type m = 0,size_type u = 0)
    {
        size_type data_offset = os.tellp();

        // compute properties
        if (m == 0) m = std::distance(begin,end);
        if (u == 0) {
            if (!t_sorted) {
                u = std::accumulate(begin,end, 0LL)+1;
            } else {
                u = *(end-1)+1;
            }
        }
        uint8_t logm = sdsl::bits::hi(m)+1;
        uint8_t logu = sdsl::bits::hi(u)+1;
        uint8_t width_low;
        if (logu < logm) {
            width_low = 1;
        } else {
            if (logm == logu) logm--;
            width_low = logu - logm;
        }

        if (!t_compact) {
            // write size
            os.encode<coder::elias_gamma>(m);
            // write universe size
            os.encode<coder::elias_gamma>(u);
        }

        // write low
        os.expand_if_needed(m*width_low);
        uint64_t last = 0;
        auto itr = begin;
        while (itr != end) {
            auto position = *itr;
            if (!t_sorted) position += last;
            last = position;
            os.put_int_no_size_check(position,width_low);
            ++itr;
        }

        // write high
        size_type num_zeros = (u >> width_low);
        os.expand_if_needed(num_zeros+m+1);
        itr = begin;
        size_type last_high=0;
        last = 0;
        while (itr != end) {
            auto position = *itr;
            if (!t_sorted) position += last;
            last = position;
            size_type cur_high = position >> width_low;
            os.put_unary_no_size_check(cur_high-last_high);
            last_high = cur_high;
            ++itr;
        }
        // terminator bit so no checks for end of bitstream
        // are needed inside the iterators
        os.put(1);

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

    static size_type estimate_size(size_type m,size_type u)
    {
        uint8_t logm = sdsl::bits::hi(m)+1;
        uint8_t logu = sdsl::bits::hi(u)+1;
        if (logm == logu) logm--;
        uint8_t width_low = logu - logm;
        return width_low*m + 2*m;
    }
};

