#include "gtest/gtest.h"
#include "bit_magic.hpp"
#include "sdsl/int_vector.hpp"
#include "bit_streams.hpp"
#include "bit_coders.hpp"
#include "list_types.hpp"
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

TEST(eliasfano, backward_iterate)
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

            auto itr = begin+20;
            for (size_t i=0; i<20; i++) {
                ASSERT_EQ(*itr,A[20-i]);
                --itr;
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

TEST(pos_intersection, simple)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 1000);
    std::uniform_int_distribution<uint64_t> ldis(1, 100);

    for (size_t i=0; i<n; i++) {
        size_t len = ldis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        size_t ln = std::distance(A.begin(),last);
        std::vector<uint32_t> B(ln);
        std::copy(A.begin(),last,B.begin());
        for (size_t j=0; j<ln; j++) B[j] = B[j]+1;

        sdsl::bit_vector bv;
        size_t offsetA,offsetB;
        {
            bit_ostream os(bv);
            offsetA = eliasfano_list<true>::create(os,A.begin(),last);
            offsetB = eliasfano_list<true>::create(os,B.begin(),B.end());
        }
        {
            bit_istream is(bv);
            auto listA = eliasfano_list<true>::materialize(is,offsetA);
            auto listB = eliasfano_list<true>::materialize(is,offsetB);

            offset_proxy_list<eliasfano_list<true>::list_type> offA(listA,0);
            offset_proxy_list<eliasfano_list<true>::list_type> offB(listB,1);

            std::vector<offset_proxy_list<eliasfano_list<true>::list_type>> lists {offA,offB};
            auto res = pos_intersect(lists);

            std::vector<uint32_t> ires;
            std::set_intersection(A.begin(),last,A.begin(),last,std::back_inserter(ires));
            ASSERT_EQ(ires.size(),res.size());
            for (size_t i=0; i<ires.size(); i++) ASSERT_EQ(ires[i],res[i]);
        }
    }
}

TEST(pos_intersection, mixed)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 10000);
    std::uniform_int_distribution<uint64_t> ldis(1, 1000);
    std::uniform_int_distribution<uint64_t> rdis(1, 100);
    std::uniform_int_distribution<uint64_t> ndis(2, 10);

    for (size_t i=0; i<n; i++) {
        // generate result
        size_t rlen = rdis(gen);
        std::vector<uint32_t> res(rlen);
        for (size_t j=0; j<rlen; j++) res[j] = dis(gen);
        std::sort(res.begin(),res.end());
        auto rlast = std::unique(res.begin(),res.end());
        auto num_res = std::distance(res.begin(),rlast);


        // generate lists and insert more integers
        std::vector<uint32_t> ires;
        std::vector<uint64_t> list_offsets;
        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto nlists = ndis(gen);
            for (size_t j=0; j<nlists; j++) {
                auto list_len = ldis(gen);
                std::vector<uint32_t> L(list_len+rlen);
                std::copy(res.begin(),rlast,L.begin());
                for (size_t x=num_res; x<L.size(); x++) L[x] = dis(gen);
                std::sort(L.begin(),L.end());
                auto llast = std::unique(L.begin(),L.end());
                if (ires.size() == 0) ires = std::vector<uint32_t>(L.begin(),llast);
                std::vector<uint32_t> iires;
                std::set_intersection(ires.begin(),ires.end(),L.begin(),llast,std::back_inserter(iires));
                ires = iires;
                for (size_t x=0; x<L.size(); x++) L[x] = L[x] + j;
                auto Loffset = eliasfano_list<true>::create(os,L.begin(),llast);
                list_offsets.push_back(Loffset);
            }
        }
        {
            bit_istream is(bv);
            std::vector<offset_proxy_list<eliasfano_list<true>::list_type>> lists;
            for (size_t j=0; j<list_offsets.size(); j++) {
                auto list = eliasfano_list<true>::materialize(is,list_offsets[j]);
                offset_proxy_list<eliasfano_list<true>::list_type> offL(list,j);
                lists.push_back(offL);
            }

            auto result = pos_intersect(lists);
            ASSERT_EQ(ires.size(),result.size());
            for (size_t i=0; i<ires.size(); i++) ASSERT_EQ(ires[i],result[i]);
        }
    }
}


TEST(bvlist, iterate)
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
            auto offset = bitvector_list<>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = bitvector_list<>::materialize(is,0);
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

TEST(bvlist, iterate_skip)
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
            auto offset = bitvector_list<>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = bitvector_list<>::materialize(is,0);
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

TEST(bvlist, skip_asign)
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
            auto offset = bitvector_list<>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = bitvector_list<>::materialize(is,0);
            auto begin = list.begin();
            ASSERT_EQ(begin.size(),ln);
            for (size_t i=5; i<ln; i+=5) {
                ASSERT_EQ(*(begin+i),A[i]);
            }
        }
    }
}

TEST(bvlist, intersection)
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
            offsetA = bitvector_list<>::create(os,A.begin(),last);
            offsetB = bitvector_list<>::create(os,B.begin(),lastB);
        }
        {
            bit_istream is(bv);
            auto listA = bitvector_list<>::materialize(is,offsetA);
            auto listB = bitvector_list<>::materialize(is,offsetB);
            auto res = intersect(listA,listB);

            std::vector<uint32_t> ires;
            std::set_intersection(A.begin(),A.end(),B.begin(),B.end(),std::back_inserter(ires));
            ASSERT_EQ(ires.size(),res.size());
            for (size_t i=0; i<ires.size(); i++) ASSERT_EQ(ires[i],res[i]);
        }
    }
}

TEST(uniform_eliasfano, small_lists_iterate)
{
    size_t n = 20;
    std::mt19937 gen(4711);
    std::uniform_int_distribution<uint64_t> dis(1, 100000);
    std::uniform_int_distribution<uint64_t> ldis(1, 128);

    for (size_t i=0; i<n; i++) {
        size_t len = ldis(gen);
        std::vector<uint32_t> A(len);
        for (size_t j=0; j<len; j++) A[j] = dis(gen);
        std::sort(A.begin(),A.end());
        auto last = std::unique(A.begin(),A.end());
        size_t ln = std::distance(A.begin(),last);

        sdsl::bit_vector bv;
        {
            bit_ostream os(bv);
            auto offset = uniform_eliasfano_list<>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = uniform_eliasfano_list<>::materialize(is,0);
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

TEST(uniform_eliasfano, iterate)
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
            auto offset = uniform_eliasfano_list<>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = uniform_eliasfano_list<>::materialize(is,0);
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

TEST(uniform_eliasfano, skip_asign)
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
            auto offset = uniform_eliasfano_list<>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }
        {
            bit_istream is(bv);
            auto list = uniform_eliasfano_list<>::materialize(is,0);
            auto begin = list.begin();
            ASSERT_EQ(begin.size(),ln);
            for (size_t i=5; i<ln; i+=5) {
                ASSERT_EQ(*(begin+i),A[i]);
            }
        }
    }
}

TEST(uniform_eliasfano, intersection)
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
            offsetA = uniform_eliasfano_list<>::create(os,A.begin(),last);
            offsetB = uniform_eliasfano_list<>::create(os,B.begin(),lastB);
        }
        {
            bit_istream is(bv);
            auto listA = uniform_eliasfano_list<>::materialize(is,offsetA);
            auto listB = uniform_eliasfano_list<>::materialize(is,offsetB);
            auto res = intersect(listA,listB);

            std::vector<uint32_t> ires;
            std::set_intersection(A.begin(),A.end(),B.begin(),B.end(),std::back_inserter(ires));
            ASSERT_EQ(ires.size(),res.size());
            for (size_t i=0; i<ires.size(); i++) ASSERT_EQ(ires[i],res[i]);
        }
    }
}


TEST(uniform_eliasfano, skip_rand_exist)
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
            auto offset = uniform_eliasfano_list<128>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }

        {
            bit_istream is(bv);
            auto list = uniform_eliasfano_list<128>::materialize(is,0);
            auto itr = list.begin();
            size_t ln = std::distance(A.begin(),last);
            for (size_t j=dis(gen)%255; j<ln; j+=(dis(gen)%255)) {
                ASSERT_TRUE(itr.skip(A[j]));
                ASSERT_EQ(*itr,A[j]);
            }
        }
    }
}

TEST(uniform_eliasfano, skip_rand_oneoff)
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
            auto offset = uniform_eliasfano_list<128>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }

        {
            bit_istream is(bv);
            auto list = uniform_eliasfano_list<128>::materialize(is,0);
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

TEST(uniform_eliasfano, skip_rand_largegaps)
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
            auto offset = uniform_eliasfano_list<128>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }

        {
            bit_istream is(bv);
            auto list = uniform_eliasfano_list<128>::materialize(is,0);
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

TEST(uniform_eliasfano, skip_rand_largegaps_hit)
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
            auto offset = uniform_eliasfano_list<128>::create(os,A.begin(),last);
            ASSERT_EQ(offset,0ULL);
        }

        {
            bit_istream is(bv);
            auto list = uniform_eliasfano_list<128>::materialize(is,0);
            auto itr = list.begin();
            size_t ln = std::distance(A.begin(),last);
            for (size_t j=1+dis(gen)%255; j<ln; j+=(dis(gen)%25)) {
                ASSERT_TRUE(itr.skip(A[j]));
                ASSERT_EQ(*itr,A[j]);
            }
        }
    }
}




int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}

