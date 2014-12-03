#pragma once

#include "sdsl/int_vector.hpp"

#include  <algorithm>

template<class t_itr>
struct list_dummy {
    using size_type = sdsl::int_vector<>::size_type;
    t_itr m_begin;
    t_itr m_end;
    size_type m_size;
    list_dummy(const t_itr& b,const t_itr& e,size_type s) : m_begin(b), m_end(e), m_size(s) {}
    list_dummy(const t_itr& b,const t_itr& e) : m_begin(b), m_end(e)
    {
        m_size = m_begin.size();
    }
    t_itr begin() const
    {
        return m_begin;
    }
    t_itr end() const
    {
        return m_end;
    }
    size_type size() const
    {
        return m_size;
    }
    bool operator<(const list_dummy<t_itr> b) const
    {
        return size() < b.size();
    }
};


struct intersection_res_itr : public std::iterator<std::random_access_iterator_tag,uint64_t,std::ptrdiff_t> {
    using size_type = sdsl::int_vector<>::size_type;
    const sdsl::int_vector<64>& m_data;
    size_type m_cur_offset;
    size_type m_size;
    intersection_res_itr(const sdsl::int_vector<64>& d,size_type offset) : m_data(d), m_cur_offset(offset)
    {
        m_size = m_data.size();
    }
    bool skip(uint64_t pos)
    {
        auto lower = std::lower_bound(m_data.begin()+m_cur_offset,m_data.end(),pos);
        m_cur_offset = 	std::distance(m_data.begin(),lower);
        if (lower == m_data.end() || *lower != pos) {
            return false;
        }
        return true;
    }
    size_type size() const
    {
        return m_size;
    }
    size_type offset() const
    {
        return m_cur_offset;
    }
    size_type remaining() const
    {
        return m_size - m_cur_offset;
    }
    uint64_t operator*() const
    {
        return m_data[m_cur_offset];
    }
    bool operator ==(const intersection_res_itr& b) const
    {
        return m_cur_offset == b.m_cur_offset;
    }
    bool operator !=(const intersection_res_itr& b) const
    {
        return m_cur_offset != b.m_cur_offset;
    }
    intersection_res_itr operator++(int)
    {
        intersection_res_itr tmp(*this);
        m_cur_offset++;
        return tmp;
    }
    intersection_res_itr& operator++()
    {
        m_cur_offset++;
        return *this;
    }
    intersection_res_itr& operator+=(size_type i)
    {
        m_cur_offset+=i;
        return *this;
    }
    intersection_res_itr operator+(size_type i)
    {
        intersection_res_itr tmp(*this);
        tmp += i;
        return tmp;
    }
    template<class t_itr>
    auto operator-(const t_itr& b) const -> difference_type
    {
        return (difference_type)offset() - (difference_type)b.offset();
    }
};

struct intersection_result {
    using size_type = sdsl::int_vector<>::size_type;
    sdsl::int_vector<64> m_data;
    intersection_result(size_type s) : m_data(s) {}
    void resize(size_type s)
    {
        m_data.resize(s);
    }
    intersection_res_itr begin() const
    {
        return intersection_res_itr(m_data,0);
    }
    intersection_res_itr end() const
    {
        return intersection_res_itr(m_data,m_data.size());
    }
    sdsl::int_vector<64>::reference operator[](const size_type& i)
    {
        return m_data[i];
    }
    size_type size() const
    {
        return m_data.size();
    }
};