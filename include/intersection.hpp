#pragma once

#include "sdsl/int_vector.hpp"

template<class t_itr,class t_itr2>
sdsl::int_vector<32>
intersect(t_itr fbegin,t_itr fend,t_itr2 sbegin,t_itr2 send,size_t offset = 0)
{
    auto n = std::distance(fbegin,fend);
    auto m = std::distance(sbegin,send);
    sdsl::int_vector<32> res(std::min(n,m));
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
        std::cout << "percent out of block = " << (float)sbegin.out_of_block_search / (float) skips << std::endl;
        std::cout << "percent in block = " << (float)sbegin.in_block_search / (float) skips << std::endl;
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
        std::cout << "percent out of block = " << (float)fbegin.out_of_block_search / (float) skips << std::endl;
        std::cout << "percent in block = " << (float)sbegin.in_block_search / (float) skips << std::endl;
    }
    res.resize(i);
    return res;
}

template<class t_list1,class t_list2>
sdsl::int_vector<32>
intersect(const t_list1& first,const t_list2& second,size_t offset = 0)
{
    return intersect(first.begin(),first.end(),second.begin(),second.end(),offset);
}