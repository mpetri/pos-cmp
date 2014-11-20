#pragma once

#include <iterator>

template<class t_itr>
struct id_range_adaptor_itr : public std::iterator<std::random_access_iterator_tag,uint64_t,std::ptrdiff_t> {
    using size_type = sdsl::int_vector<>::size_type;
    t_itr m_itr;
    t_itr m_end;
    id_range_adaptor_itr(t_itr itr,t_itr end) : m_itr(itr), m_end(end) {}
    id_range_adaptor_itr& operator++()
    {
        if (m_itr != m_end) {
            auto cur = *m_itr;
            while (m_itr != m_end && *m_itr == cur) {
                m_itr++;
            }
        }
        return *this;
    }
    uint64_t operator*() const
    {
        return *m_itr;
    }
    bool operator ==(const id_range_adaptor_itr& b) const
    {
        return m_itr == b.m_itr;
    }
    bool operator !=(const id_range_adaptor_itr& b) const
    {
        return m_itr != b.m_itr;
    }
    id_range_adaptor_itr& operator+=(size_type i)
    {
        for (size_t j=0; j<i; j++) {
            ++(*this);
        }
        return *this;
    }
    id_range_adaptor_itr operator+(size_type i)
    {
        id_range_adaptor_itr tmp(*this);
        tmp += i;
        return tmp;
    }
    template<class T>
    auto operator-(const T& b) const -> difference_type
    {
        difference_type dist = 0;
        if (m_itr < b.m_itr) {
            auto tmp = *this;
            while (tmp != b) {
                ++tmp;
                dist--;
            }
        } else {
            auto tmp = b;
            while (tmp != *this) {
                ++tmp;
                dist++;
            }
        }
        return dist;
    }
};


template<class t_itr>
struct id_range_adaptor {
    using size_type = sdsl::int_vector<>::size_type;
    t_itr m_begin;
    t_itr m_end;
    id_range_adaptor(t_itr begin,t_itr end) : m_begin(begin), m_end(end) {}
    id_range_adaptor_itr<t_itr> begin() const
    {
        return id_range_adaptor_itr<t_itr>(m_begin,m_end);
    }
    id_range_adaptor_itr<t_itr> end() const
    {
        return id_range_adaptor_itr<t_itr>(m_end,m_end);
    }
};

template<class t_itr>
struct freq_range_adaptor_itr : public std::iterator<std::random_access_iterator_tag,uint64_t,std::ptrdiff_t> {
    using size_type = sdsl::int_vector<>::size_type;
    t_itr m_itr;
    t_itr m_end;
    uint64_t cur_freq;
    freq_range_adaptor_itr(t_itr itr,t_itr end) : m_itr(itr) , m_end(end)
    {
        cur_freq = 0;
        auto tmp = m_itr;
        if (tmp != m_end) {
            auto cur = *tmp;
            while (tmp != m_end && *tmp == cur) {
                tmp++;
                cur_freq++;
            }
        }
    }
    freq_range_adaptor_itr& operator++()
    {
        if (m_itr != m_end) {
            m_itr += cur_freq;
            auto tmp = m_itr;
            auto cur = *tmp;
            cur_freq = 0;
            while (tmp != m_end && *tmp == cur) {
                tmp++;
                cur_freq++;
            }
        }
        return *this;
    }
    uint64_t operator*() const
    {
        return cur_freq;
    }
    bool operator ==(const freq_range_adaptor_itr& b) const
    {
        return m_itr == b.m_itr;
    }
    bool operator !=(const freq_range_adaptor_itr& b) const
    {
        return m_itr != b.m_itr;
    }
    freq_range_adaptor_itr& operator+=(size_type i)
    {
        for (size_t j=0; j<i; j++) {
            ++(*this);
        }
        return *this;
    }
    freq_range_adaptor_itr operator+(size_type i)
    {
        freq_range_adaptor_itr tmp(*this);
        tmp += i;
        return tmp;
    }
    template<class T>
    auto operator-(const T& b) const -> difference_type
    {
        difference_type dist = 0;
        if (m_itr < b.m_itr) {
            auto tmp = *this;
            while (tmp != b) {
                ++tmp;
                dist--;
            }
        } else {
            auto tmp = b;
            while (tmp != *this) {
                ++tmp;
                dist++;
            }
        }
        return dist;
    }
};

template<class t_itr>
struct freq_range_adaptor {
    using size_type = sdsl::int_vector<>::size_type;
    t_itr m_begin;
    t_itr m_end;
    freq_range_adaptor(t_itr begin,t_itr end) : m_begin(begin), m_end(end) {}
    freq_range_adaptor_itr<t_itr> begin() const
    {
        return freq_range_adaptor_itr<t_itr>(m_begin,m_end);
    }
    freq_range_adaptor_itr<t_itr> end() const
    {
        return freq_range_adaptor_itr<t_itr>(m_end,m_end);
    }
};
