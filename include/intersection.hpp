#pragma once

#include "sdsl/int_vector.hpp"

struct intersect_result {

};

template<class t_itr,class t_itr2>
intersection_result
intersect(t_itr fbegin,t_itr fend,t_itr2 sbegin,t_itr2 send,size_t offset = 0)
{
    auto n = std::distance(fbegin,fend);
    auto m = std::distance(sbegin,send);
    intersection_result res(std::min(n,m));
    size_t i=0;
    size_t skips = 0;
    if (n < m) {
        while (fbegin != fend) {
            auto cur = *fbegin;
            skips++;
            if (sbegin.skip(cur+offset)) {
                res[i++] = cur;
            }
            if (sbegin == send) break;
            ++fbegin;
        }
    } else {
        while (sbegin != send) {
            auto cur = *sbegin;
            skips++;
            if (fbegin.skip(cur-offset)) {
                res[i++] = cur;
            }
            if (fbegin == fend) break;
            ++sbegin;
        }
    }
    res.resize(i);
    return res;
}

template<class t_list1,class t_list2>
intersection_result
intersect(const t_list1& first,const t_list2& second,size_t offset = 0)
{
    return intersect(first.begin(),first.end(),second.begin(),second.end(),offset);
}


template<class t_list>
intersection_result
intersect(std::vector<t_list> lists)
{
    // sort by size
    std::sort(lists.begin(),lists.end());

    // perform SvS intersection
    auto res = intersect(lists[0],lists[1]);
    for (size_t i=2; i<lists.size(); i++) {
        res = intersect(res,lists[i]);
    }
    return res;
}

