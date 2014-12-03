#include "gtest/gtest.h"
#include "bit_magic.hpp"
#include "sdsl/int_vector.hpp"
#include "bit_streams.hpp"
#include "bit_coders.hpp"
#include "eliasfano_list.hpp"
#include "optpfor_list.hpp"
#include "intersection.hpp"

#include <functional>
#include <random>

TEST(bit_magic, next0rand)
{
    size_t n = 10;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(0, 0xFFFFFFFFFFFFFFFF);
    sdsl::bit_vector bv(64*n);
    for (size_t i=0; i<n; i++) bv.data()[i] = dis(gen);

    size_t idx = 0;
    while (idx < bv.size()) {
        size_t next = bit_magic::next0(bv.data(),idx);
        for (size_t i=idx+1; i<next; i++) {
            if (bv[i]!=1) {
                std::cerr << "ERROR at idx=" << idx << " next=" << next << " i=" << i << std::endl;
            }
            ASSERT_EQ(bv[i],1);
        }
        idx = next;
    }
}

TEST(bit_magic, next0almostfull)
{
    size_t n = 1000;
    sdsl::bit_vector bv(64*n);
    for (size_t i=0; i<n; i++) bv.data()[i] = 0xFFFFFFFFFFFFFFFF;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(0,bv.size()-1);
    for (size_t i=0; i<100; i++) bv[dis(gen)] = 0;
    size_t idx = 0;
    while (idx < bv.size()) {
        size_t next = bit_magic::next0(bv.data(),idx);
        for (size_t i=idx+1; i<next; i++) {
            if (bv[i]!=1) {
                std::cerr << "ERROR at idx=" << idx << " next=" << next << " i=" << i << std::endl;
            }
            ASSERT_EQ(bv[i],1);
        }
        idx = next;
    }
}


TEST(bit_magic, next0full)
{
    size_t n = 1000;
    sdsl::bit_vector bv(64*n);
    for (size_t i=0; i<n; i++) bv.data()[i] = 0xFFFFFFFFFFFFFFFF;
    size_t idx = 0;
    while (idx < bv.size()) {
        size_t next = bit_magic::next0(bv.data(),idx);
        ASSERT_EQ(next,bv.size());
        idx = next;
    }
}

TEST(bit_magic, next0empty)
{
    size_t n = 1000;
    sdsl::bit_vector bv(64*n);
    for (size_t i=0; i<n; i++) bv.data()[i] = 0;
    size_t idx = 0;
    while (idx < bv.size()) {
        size_t next = bit_magic::next0(bv.data(),idx);
        ASSERT_EQ(next,idx+1);
        idx = next;
    }
}

TEST(bit_magic, nextXth0rand)
{
    size_t n = 1000;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(0, 0xFFFFFFFFFFFFFFFF);
    sdsl::bit_vector bv(64*n);

    for (size_t i=0; i<n; i++) bv.data()[i] = dis(gen);

    for (size_t i=0; i<1000; i++) {
        size_t idx = dis(gen) % bv.size();
        size_t stop = dis(gen) % bv.size();
        if (stop < idx) std::swap(idx,stop);
        if (idx == stop) continue;
        size_t next = bit_magic::next0(bv.data(),stop);
        size_t x = 0;
        for (size_t j=idx+1; j<=next; j++)
            if (bv[j]==0) x++;

        auto calc = bit_magic::next_Xth_zero(bv.data(),idx,x);
        ASSERT_EQ(calc,next);
    }
}

TEST(bit_magic, nextXth0empty)
{
    size_t n = 1000;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(0, 0xFFFFFFFFFFFFFFFF);
    sdsl::bit_vector bv(64*n);
    for (size_t i=0; i<n; i++) bv.data()[i] = 0;
    for (size_t i=0; i<1000; i++) {
        size_t idx = dis(gen) % bv.size();
        size_t stop = dis(gen) % bv.size();
        if (stop < idx) std::swap(idx,stop);
        if (idx == stop) continue;
        size_t x = stop-idx;
        size_t next = bit_magic::next_Xth_zero(bv.data(),idx,x);
        ASSERT_EQ(next,stop);
    }
}

TEST(bit_magic, nextXth0almostfull)
{
    size_t n = 1000;
    sdsl::bit_vector bv(64*n);
    for (size_t i=0; i<n; i++) bv.data()[i] = 0xFFFFFFFFFFFFFFFF;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(0,bv.size()-1);
    for (size_t i=0; i<100; i++) bv[dis(gen)] = 0;
    for (size_t i=0; i<1000; i++) {
        size_t idx = dis(gen) % bv.size();
        size_t zeros_to_left = 0;
        for (size_t j=idx+1; j<bv.size(); j++) {
            if (bv[j]==0) zeros_to_left++;
        }
        if (zeros_to_left == 0) continue;
        size_t x = 1 + dis(gen) % zeros_to_left;
        size_t next = bit_magic::next_Xth_zero(bv.data(),idx,x);
        size_t zero_cnt = 0;
        for (size_t j=idx+1; j<=next; j++) {
            if (bv[j] == 0) zero_cnt++;
        }
        ASSERT_EQ(zero_cnt,x);
    }
}



TEST(bit_magic, nextXth1rand)
{
    size_t n = 1000;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(0, 0xFFFFFFFFFFFFFFFF);
    sdsl::bit_vector bv(64*n);

    for (size_t i=0; i<n; i++) bv.data()[i] = dis(gen);

    for (size_t i=0; i<1000; i++) {
        size_t idx = dis(gen) % bv.size();
        size_t stop = dis(gen) % bv.size();
        if (stop < idx) std::swap(idx,stop);
        if (idx == stop) continue;
        size_t next = sdsl::bits::next(bv.data(),stop);
        size_t x = 0;
        for (size_t j=idx+1; j<=next; j++)
            if (bv[j]==1) x++;

        auto calc = bit_magic::next_Xth_one(bv.data(),idx,x);
        ASSERT_EQ(calc,next);
    }
}

TEST(bit_magic, nextXth1full)
{
    size_t n = 1000;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(0, 0xFFFFFFFFFFFFFFFF);
    sdsl::bit_vector bv(64*n);
    for (size_t i=0; i<n; i++) bv.data()[i] = 0xFFFFFFFFFFFFFFFF;
    for (size_t i=0; i<1000; i++) {
        size_t idx = dis(gen) % bv.size();
        size_t stop = dis(gen) % bv.size();
        if (stop < idx) std::swap(idx,stop);
        if (idx == stop) continue;
        size_t x = stop-idx;
        size_t next = bit_magic::next_Xth_one(bv.data(),idx,x);
        ASSERT_EQ(next,stop);
    }
}

TEST(bit_magic, nextXth1almostempty)
{
    size_t n = 1000;
    sdsl::bit_vector bv(64*n);
    for (size_t i=0; i<n; i++) bv.data()[i] = 0;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(0,bv.size()-1);
    for (size_t i=0; i<100; i++) bv[dis(gen)] = 1;
    for (size_t i=0; i<1000; i++) {
        size_t idx = dis(gen) % bv.size();
        size_t ones_to_left = 0;
        for (size_t j=idx+1; j<bv.size(); j++) {
            if (bv[j]==1) ones_to_left++;
        }
        if (ones_to_left == 0) continue;
        size_t x = 1 + dis(gen) % ones_to_left;
        size_t next = bit_magic::next_Xth_one(bv.data(),idx,x);
        size_t one_cnt = 0;
        for (size_t j=idx+1; j<=next; j++) {
            if (bv[j] == 1) one_cnt++;
        }
        ASSERT_EQ(one_cnt,x);
    }
}


TEST(bit_stream, unary)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        auto n = std::distance(A.begin(),last);
        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            os.write_unary(A.begin(),n);
        }
        std::vector<uint32_t> B(n);
        {
            bit_istream is(bv);
            is.get_unary(B.begin(),n);
        }
        for (auto i=0; i<n; i++) {
            ASSERT_EQ(B[i],A[i]);
        }
    }
}


TEST(bit_stream, vbyte)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        auto n = std::distance(A.begin(),last);
        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            os.encode<coder::vbyte>(A.begin(),n);
        }
        std::vector<uint32_t> B(n);
        {
            bit_istream is(bv);
            is.decode<coder::vbyte>(B.begin(),n);
        }
        for (auto i=0; i<n; i++) {
            ASSERT_EQ(B[i],A[i]);
        }
    }
}

TEST(bit_stream, fix_width_int)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        auto n = std::distance(A.begin(),last);
        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            os.write_int(A.begin(),n,7);
        }
        std::vector<uint32_t> B(n);
        {
            bit_istream is(bv);
            is.get_int(B.begin(),n,7);
        }
        for (auto i=0; i<n; i++) {
            ASSERT_EQ(B[i],A[i]);
        }
    }
}

TEST(bit_stream, gamma)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        auto n = std::distance(A.begin(),last);
        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            os.encode<coder::elias_gamma>(A.begin(),last);
        }
        std::vector<uint32_t> B(n);
        {
            bit_istream is(bv);
            is.decode<coder::elias_gamma>(B.begin(),n);
        }
        for (auto i=0; i<n; i++) {
            ASSERT_EQ(B[i],A[i]);
        }
    }
}

TEST(bit_stream, delta)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        auto n = std::distance(A.begin(),last);
        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            os.encode<coder::elias_delta>(A.begin(),last);
        }
        std::vector<uint32_t> B(n);
        {
            bit_istream is(bv);
            is.decode<coder::elias_delta>(B.begin(),n);
        }
        for (auto i=0; i<n; i++) {
            ASSERT_EQ(B[i],A[i]);
        }
    }
}


TEST(bit_stream, delta_gamma)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        auto n = std::distance(A.begin(),last);
        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            os.encode<coder::delta<coder::elias_gamma>>(A.begin(),last);
        }
        std::vector<uint32_t> B(n);
        {
            bit_istream is(bv);
            is.decode<coder::delta<coder::elias_gamma>>(B.begin(),n);
        }
        for (auto i=0; i<n; i++) {
            ASSERT_EQ(B[i],A[i]);
        }
    }
}

TEST(bit_stream, delta_vbyte)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        auto n = std::distance(A.begin(),last);
        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            os.encode<coder::delta<coder::vbyte>>(A.begin(),last);
        }
        std::vector<uint32_t> B(n);
        {
            bit_istream is(bv);
            is.decode<coder::delta<coder::vbyte>>(B.begin(),n);
        }
        for (auto i=0; i<n; i++) {
            ASSERT_EQ(B[i],A[i]);
        }
    }
}

TEST(eliasfano, iterate)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        size_t ln = std::distance(A.begin(),last);

        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = eliasfano_list<true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = eliasfano_list<true>::materialize(is,0);
            auto begin = list.begin();
            auto end = list.end();
            ASSERT_EQ(begin.size(),ln);

            size_t i = 0;
            while (begin != end) {
                ASSERT_EQ(*begin,A[i]);
                i++;
                ++begin;
            }
        }
    }
}

TEST(eliasfano, iterate_skip)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        size_t ln = std::distance(A.begin(),last);

        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = eliasfano_list<true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = eliasfano_list<true>::materialize(is,0);
            auto begin = list.begin();
            auto end = list.end();
            ASSERT_EQ(begin.size(),ln);

            size_t i = 0;
            auto tmp = begin;
            while (begin != end) {
                ASSERT_EQ(*begin,A[i]);
                i += 5;
                tmp = begin;
                begin += 5;
                if (i >= begin.size()) break;
            }
        }
    }
}

TEST(eliasfano, skip_asign)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        size_t ln = std::distance(A.begin(),last);

        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = eliasfano_list<true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = eliasfano_list<true>::materialize(is,0);
            auto begin = list.begin();
            ASSERT_EQ(begin.size(),ln);
            for (size_t i=5; i<ln; i+=5) {
                ASSERT_EQ(*(begin+i),A[i]);
            }
        }
    }
}

TEST(eliasfano, unsorted_iterate)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);
    std::uniform_int_distribution<uint64_t> vdis(1, 100);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = vdis(gen);
        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = eliasfano_list<false>::create(os,A.begin(),A.end());
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = eliasfano_list<false>::materialize(is,0);
            auto begin = list.begin();
            auto end = list.end();
            ASSERT_EQ(begin.size(),len);

            size_t i = 0;
            while (begin != end) {
                ASSERT_EQ(*begin,A[i]);
                i++;
                ++begin;
            }
        }
    }
}

TEST(eliasfano, unsorted_iterate_skip)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);
    std::uniform_int_distribution<uint64_t> vdis(1, 100);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = vdis(gen);
        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = eliasfano_list<false>::create(os,A.begin(),A.end());
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = eliasfano_list<false>::materialize(is,0);
            auto begin = list.begin();
            auto end = list.end();
            ASSERT_EQ(begin.size(),len);

            size_t i = 0;
            auto tmp = begin;
            while (begin != end) {
                ASSERT_EQ(*begin,A[i]);
                i += 5;
                tmp = begin;
                begin += 5;
                if (i >= begin.size()) break;
            }
        }
    }
}


TEST(eliasfano, skip_rand_exist)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());

        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = eliasfano_list<true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }

        {
            bit_istream is(bv);
            auto list = eliasfano_list<true>::materialize(is,0);
            auto itr = list.begin();
            size_t ln = std::distance(A.begin(),last);
            for (size_t j=dis(gen)%255; j<ln; j+=(dis(gen)%255)) {
                ASSERT_TRUE(itr.skip(A[j]));
                ASSERT_EQ(*itr,A[j]);
            }
        }
    }
}

TEST(eliasfano, skip_rand_oneoff)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());

        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = eliasfano_list<true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }

        {
            bit_istream is(bv);
            auto list = eliasfano_list<true>::materialize(is,0);
            auto itr = list.begin();
            size_t ln = std::distance(A.begin(),last);
            for (size_t j=1+dis(gen)%255; j<ln; j+=(dis(gen)%25)) {
                if (A[j-1] == A[j]-1) continue;
                ASSERT_FALSE(itr.skip(A[j]-1));
                ASSERT_EQ(*itr,A[j]);
            }
        }
    }
}

TEST(eliasfano, skip_rand_largegaps)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);
    std::uniform_int_distribution<uint64_t> ldis(1, 100);

    for (size_t i=0; i<n; i++) {
        size_t len = ldis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());

        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = eliasfano_list<true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }

        {
            bit_istream is(bv);
            auto list = eliasfano_list<true>::materialize(is,0);
            auto itr = list.begin();
            size_t ln = std::distance(A.begin(),last);
            for (size_t j=1+dis(gen)%255; j<ln; j+=(dis(gen)%25)) {
                if (A[j-1] == A[j]-1) continue;
                ASSERT_FALSE(itr.skip(A[j]-1));
                ASSERT_EQ(*itr,A[j]);
            }
        }
    }
}

TEST(eliasfano, skip_rand_largegaps_hit)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 1000000);
    std::uniform_int_distribution<uint64_t> ldis(1, 1000);

    for (size_t i=0; i<n; i++) {
        size_t len = ldis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());


        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = eliasfano_list<true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }

        {
            bit_istream is(bv);
            auto list = eliasfano_list<true>::materialize(is,0);
            auto itr = list.begin();
            size_t ln = std::distance(A.begin(),last);
            for (size_t j=1+dis(gen)%255; j<ln; j+=(dis(gen)%25)) {
                ASSERT_TRUE(itr.skip(A[j]));
                ASSERT_EQ(*itr,A[j]);
            }
        }
    }
}


TEST(optpfor, iterate)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        size_t ln = std::distance(A.begin(),last);

        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = optpfor_list<128,true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = optpfor_list<128,true>::materialize(is,0);
            auto begin = list.begin();
            auto end = list.end();
            ASSERT_EQ(begin.size(),ln);

            size_t i = 0;
            while (begin != end) {
                ASSERT_EQ(*begin,A[i]);
                i++;
                ++begin;
            }
        }
    }
}

TEST(optpfor, iterate_skip)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        size_t ln = std::distance(A.begin(),last);

        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = optpfor_list<128,true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = optpfor_list<128,true>::materialize(is,0);
            auto begin = list.begin();
            auto end = list.end();
            ASSERT_EQ(begin.size(),ln);

            size_t i = 0;
            auto tmp = begin;
            while (begin != end) {
                ASSERT_EQ(*begin,A[i]);
                i += 5;
                tmp = begin;
                begin += 5;
                if (i >= begin.size()) break;
            }
        }
    }
}

TEST(optpfor, skip_asign)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        size_t ln = std::distance(A.begin(),last);

        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = optpfor_list<128,true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = optpfor_list<128,true>::materialize(is,0);
            auto begin = list.begin();
            ASSERT_EQ(begin.size(),ln);
            for (size_t i=5; i<ln; i+=5) {
                ASSERT_EQ(*(begin+i),A[i]);
            }
        }
    }
}

TEST(optpfor, unsorted_iterate)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);
    std::uniform_int_distribution<uint64_t> vdis(1, 100);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = vdis(gen);
        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = optpfor_list<128,false>::create(os,A.begin(),A.end());
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = optpfor_list<128,false>::materialize(is,0);
            auto begin = list.begin();
            auto end = list.end();
            ASSERT_EQ(begin.size(),len);

            size_t i = 0;
            while (begin != end) {
                ASSERT_EQ(*begin,A[i]);
                i++;
                ++begin;
            }
        }
    }
}

TEST(optpfor, unsorted_iterate_skip)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);
    std::uniform_int_distribution<uint64_t> vdis(1, 100);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = vdis(gen);
        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = optpfor_list<128,false>::create(os,A.begin(),A.end());
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = optpfor_list<128,false>::materialize(is,0);
            auto begin = list.begin();
            auto end = list.end();
            ASSERT_EQ(begin.size(),len);

            size_t i = 0;
            auto tmp = begin;
            while (begin != end) {
                ASSERT_EQ(*begin,A[i]);
                i += 5;
                tmp = begin;
                begin += 5;
                if (i >= begin.size()) break;
            }
        }
    }
}



TEST(optpfor, skip_rand_exist)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());

        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = optpfor_list<128,true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }

        {
            bit_istream is(bv);
            auto list = optpfor_list<128,true>::materialize(is,0);
            auto itr = list.begin();
            size_t ln = std::distance(A.begin(),last);
            for (size_t j=dis(gen)%255; j<ln; j+=(dis(gen)%255)) {
                ASSERT_TRUE(itr.skip(A[j]));
                ASSERT_EQ(*itr,A[j]);
            }
        }
    }
}

TEST(optpfor, skip_rand_oneoff)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);

    for (size_t i=0; i<n; i++) {
        size_t len = dis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());

        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = optpfor_list<128,true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }

        {
            bit_istream is(bv);
            auto list = optpfor_list<128,true>::materialize(is,0);
            auto itr = list.begin();
            size_t ln = std::distance(A.begin(),last);
            for (size_t j=1+dis(gen)%255; j<ln; j+=(dis(gen)%25)) {
                if (A[j-1] == A[j]-1) continue;
                ASSERT_FALSE(itr.skip(A[j]-1));
                ASSERT_EQ(*itr,A[j]);
            }
        }
    }
}

TEST(optpfor, skip_rand_largegaps)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);
    std::uniform_int_distribution<uint64_t> ldis(1, 100);

    for (size_t i=0; i<n; i++) {
        size_t len = ldis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());

        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = optpfor_list<128,true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }

        {
            bit_istream is(bv);
            auto list = optpfor_list<128,true>::materialize(is,0);
            auto itr = list.begin();
            size_t ln = std::distance(A.begin(),last);
            for (size_t j=1+dis(gen)%255; j<ln; j+=(dis(gen)%25)) {
                if (A[j-1] == A[j]-1) continue;
                ASSERT_FALSE(itr.skip(A[j]-1));
                ASSERT_EQ(*itr,A[j]);
            }
        }
    }
}

TEST(optpfor, skip_rand_largegaps_hit)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 1000000);
    std::uniform_int_distribution<uint64_t> ldis(1, 1000);

    for (size_t i=0; i<n; i++) {
        size_t len = ldis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());


        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = optpfor_list<128,true>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }

        {
            bit_istream is(bv);
            auto list = optpfor_list<128,true>::materialize(is,0);
            auto itr = list.begin();
            size_t ln = std::distance(A.begin(),last);
            for (size_t j=1+dis(gen)%255; j<ln; j+=(dis(gen)%25)) {
                ASSERT_TRUE(itr.skip(A[j]));
                ASSERT_EQ(*itr,A[j]);
            }
        }
    }
}


TEST(intersection, eliasfano)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 1000000);
    std::uniform_int_distribution<uint64_t> ldis(1, 10000);

    for (size_t i=0; i<n; i++) {
        size_t len = ldis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());

        size_t len2 = ldis(gen);
        std::vector<uint32_t> B(len2);
        for (size_t j=0; j<len2; j++) B[j] = dis(gen);
        std::sort(B.begin(),B.end());
        auto lastB = std::unique(B.begin(),B.end());

        sdsl::bit_vector bv;
        size_t offsetA,offsetB;
        {
            bit_ostream os(bv);
            offsetA = eliasfano_list<true>::create(os,A.begin(),last);
            offsetB = eliasfano_list<true>::create(os,B.begin(),lastB);
        }
        {
            bit_istream is(bv);
            auto listA = eliasfano_list<true>::materialize(is,offsetA);
            auto listB = eliasfano_list<true>::materialize(is,offsetB);
            auto res = intersect(listA,listB);

            std::vector<uint32_t> ires;
            std::set_intersection(A.begin(),A.end(),B.begin(),B.end(),std::back_inserter(ires));
            ASSERT_EQ(ires.size(),res.size());
            for (size_t i=0; i<ires.size(); i++) ASSERT_EQ(ires[i],res[i]);
        }
    }
}

TEST(intersection, optpfor)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 1000000);
    std::uniform_int_distribution<uint64_t> ldis(1, 10000);

    for (size_t i=0; i<n; i++) {
        size_t len = ldis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());

        size_t len2 = ldis(gen);
        std::vector<uint32_t> B(len2);
        for (size_t j=0; j<len2; j++) B[j] = dis(gen);
        std::sort(B.begin(),B.end());
        auto lastB = std::unique(B.begin(),B.end());

        sdsl::bit_vector bv;
        size_t offsetA,offsetB;
        {
            bit_ostream os(bv);
            offsetA = optpfor_list<128,true>::create(os,A.begin(),last);
            offsetB = optpfor_list<128,true>::create(os,B.begin(),lastB);
        }
        {
            bit_istream is(bv);
            auto listA = optpfor_list<128,true>::materialize(is,offsetA);
            auto listB = optpfor_list<128,true>::materialize(is,offsetB);
            auto res = intersect(listA,listB);

            std::vector<uint32_t> ires;
            std::set_intersection(A.begin(),A.end(),B.begin(),B.end(),std::back_inserter(ires));
            ASSERT_EQ(ires.size(),res.size());
            for (size_t i=0; i<ires.size(); i++) ASSERT_EQ(ires[i],res[i]);
        }
    }
}

TEST(intersection, mixed)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 1000000);
    std::uniform_int_distribution<uint64_t> ldis(1, 10000);

    for (size_t i=0; i<n; i++) {
        size_t len = ldis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());

        size_t len2 = ldis(gen);
        std::vector<uint32_t> B(len2);
        for (size_t j=0; j<len2; j++) B[j] = dis(gen);
        std::sort(B.begin(),B.end());
        auto lastB = std::unique(B.begin(),B.end());

        sdsl::bit_vector bv;
        size_t offsetA,offsetB;
        {
            bit_ostream os(bv);
            offsetA = optpfor_list<128,true>::create(os,A.begin(),last);
            offsetB = eliasfano_list<true>::create(os,B.begin(),lastB);
        }
        {
            bit_istream is(bv);
            auto listA = optpfor_list<128,true>::materialize(is,offsetA);
            auto listB = eliasfano_list<true>::materialize(is,offsetB);
            auto res = intersect(listA,listB);

            std::vector<uint32_t> ires;
            std::set_intersection(A.begin(),A.end(),B.begin(),B.end(),std::back_inserter(ires));
            ASSERT_EQ(ires.size(),res.size());
            for (size_t i=0; i<ires.size(); i++) ASSERT_EQ(ires[i],res[i]);
        }
    }
}


TEST(intersection, multiple)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 1000);
    std::uniform_int_distribution<uint64_t> ldis(1, 10000);

    for (size_t i=0; i<n; i++) {
        std::vector<size_t> list_offsets;
        sdsl::bit_vector bv;
        std::vector<uint32_t> I;
        {
            bit_ostream os(bv);
            for (size_t j=0; j<5; j++) {
                size_t len = ldis(gen);
                std::vector<uint32_t> A(len);
                for (size_t j=0; j<len; j++) A[j] = dis(gen);
                std::sort(A.begin(),A.end());
                auto last = std::unique(A.begin(),A.end());
                size_t offset = optpfor_list<128,true>::create(os,A.begin(),last);
                list_offsets.push_back(offset);

                if (j==0) I = A;
                else {
                    std::vector<uint32_t> ires;
                    std::set_intersection(A.begin(),last,I.begin(),I.end(),std::back_inserter(ires));
                    I = ires;
                }
            }
        }
        {
            std::vector< optpfor_list<128,true>::list_type > lists;
            bit_istream is(bv);
            for (const auto& o : list_offsets) {
                lists.emplace_back(optpfor_list<128,true>::materialize(is,o));
            }

            auto res = intersect(lists);
            ASSERT_EQ(I.size(),res.size());
            for (size_t i=0; i<I.size(); i++) ASSERT_EQ(I[i],res[i]);
        }
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}

