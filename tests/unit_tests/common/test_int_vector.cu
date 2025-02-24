/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <gridtools/common/int_vector.hpp>

#include <type_traits>

#include <gtest/gtest.h>

#include <gridtools/common/hymap.hpp>
#include <gridtools/common/integral_constant.hpp>
#include <gridtools/common/tuple_util.hpp>

#include <cuda_test_helper.hpp>

namespace gridtools {
    namespace {
        struct a;
        struct b;
        struct c;

        __device__ hymap::keys<a, b, c>::values<int, long int, unsigned int> plus_device(
            hymap::keys<a, b>::values<int, long int> const &m1,
            hymap::keys<b, c>::values<int, unsigned int> const &m2) {
            return int_vector::plus(m1, m2);
        }

        TEST(plus, device) {
            auto m1 = tuple_util::make<hymap::keys<a, b>::values>(1, 2l);
            auto m2 = tuple_util::make<hymap::keys<b, c>::values>(10, 20u);

            auto testee = on_device::exec(GT_MAKE_INTEGRAL_CONSTANT_FROM_VALUE(&plus_device), m1, m2);

            static_assert(std::is_same<int, std::decay_t<decltype(at_key<a>(testee))>>::value);
            static_assert(std::is_same<long int, std::decay_t<decltype(at_key<b>(testee))>>::value);
            static_assert(std::is_same<unsigned int, std::decay_t<decltype(at_key<c>(testee))>>::value);

            EXPECT_EQ(1, at_key<a>(testee));
            EXPECT_EQ(12, at_key<b>(testee));
            EXPECT_EQ(20, at_key<c>(testee));
        }

        __device__ hymap::keys<a, b>::values<int, int> multiply_device(
            hymap::keys<a, b>::values<integral_constant<int, 1>, int> const &vec, int scalar) {
            return int_vector::multiply(vec, scalar);
        }

        TEST(multiply, device) {
            auto vec = tuple_util::make<hymap::keys<a, b>::values>(integral_constant<int, 1>{}, 2);

            auto testee = on_device::exec(GT_MAKE_INTEGRAL_CONSTANT_FROM_VALUE(&multiply_device), vec, 2);

            EXPECT_EQ(2, at_key<a>(testee));
            EXPECT_EQ(4, at_key<b>(testee));
        }

        __device__ hymap::keys<a, c>::values<int, integral_constant<int, 2>> normalize_device(
            hymap::keys<a, b, c>::values<int, integral_constant<int, 0>, integral_constant<int, 2>> const &vec) {
            return int_vector::prune_zeros(vec);
        }

        TEST(prune_zeros, device) {
            auto vec = tuple_util::make<hymap::keys<a, b, c>::values>(
                1, integral_constant<int, 0>{}, integral_constant<int, 2>{});

            auto testee = on_device::exec(GT_MAKE_INTEGRAL_CONSTANT_FROM_VALUE(&normalize_device), vec);

            EXPECT_EQ(1, at_key<a>(testee));
            EXPECT_FALSE((has_key<decltype(testee), b>{}));
            static_assert(std::is_same<integral_constant<int, 2>, std::decay_t<decltype(at_key<c>(testee))>>::value);
        }

    } // namespace
} // namespace gridtools

#include "test_int_vector.cpp"
