/**
   @file
   @brief File containing tests for the define_cache construct
*/

#include "gtest/gtest.h"
#include <boost/mpl/equal.hpp>
#include "stencil-composition/stencil-composition.hpp"
#include "stencil-composition/caches/define_caches.hpp"

using namespace gridtools;
using namespace enumtype;

#ifdef __CUDACC__
#define BACKEND backend< Cuda, GRIDBACKEND, Block >
#else
  #ifdef BACKEND_BLOCK
#define BACKEND backend< Host, GRIDBACKEND, Block >
  #else
#define BACKEND backend< Host, GRIDBACKEND, Naive >
  #endif
#endif

TEST(define_caches, test_sequence_caches)
{
#ifdef __CUDACC__
    typedef gridtools::layout_map<2,1,0> layout_t;//stride 1 on i
#else
    typedef gridtools::layout_map<0,1,2> layout_t;//stride 1 on k
#endif
    typedef gridtools::BACKEND::storage_type<float_type, gridtools::BACKEND::storage_info<0,layout_t> >::type storage_type;

    typedef gridtools::arg<0,storage_type> arg0_t;
    typedef gridtools::arg<1,storage_type> arg1_t;
    typedef gridtools::arg<2,storage_type> arg2_t;

    typedef decltype(gridtools::define_caches(
        cache<IJ, fill>(arg0_t()),
        cache<IJK, flush>(arg1_t()),
        cache<K, local>(arg2_t())
    ) ) cache_sequence_t;

    GRIDTOOLS_STATIC_ASSERT((
        boost::mpl::equal<
            cache_sequence_t,
            boost::mpl::vector3<detail::cache_impl<IJ, arg0_t, fill, USHRT_MAX, 0>, detail::cache_impl<IJK, arg1_t, flush, USHRT_MAX, 0>, detail::cache_impl<K, arg2_t, local, USHRT_MAX, 0> >
        >::value),
        "Failed TEST"
    );

#ifdef CXX11_ENABLED
	typedef decltype(gridtools::cache<IJ, fill, 3, 2>(arg0_t(), arg1_t(), arg2_t())) caches_ret_sequence_3_t;
	typedef decltype(gridtools::cache<IJK, fill, 4, 5>(arg0_t(), arg1_t())) caches_ret_sequence_2_t;
	typedef decltype(gridtools::cache<IJ, fill, 1, 10>(arg0_t())) caches_ret_sequence_1_t;

	static_assert((
		boost::mpl::equal<
		caches_ret_sequence_3_t,
		boost::mpl::vector3<detail::cache_impl<IJ, arg0_t, fill, 3, 2>, detail::cache_impl<IJ, arg1_t, fill, 3, 2>, detail::cache_impl<IJ, arg2_t, fill, 3, 2> >
		>::value),
		"Failed TEST"
		);
	static_assert((
		boost::mpl::equal<
		caches_ret_sequence_2_t,
		boost::mpl::vector2<detail::cache_impl<IJK, arg0_t, fill, 4, 5>, detail::cache_impl<IJK, arg1_t, fill, 4, 5> >
		>::value),
		"Failed TEST"
		);
	static_assert((
		boost::mpl::equal<
		caches_ret_sequence_1_t,
		boost::mpl::vector1<detail::cache_impl<IJ, arg0_t, fill, 1, 10> >
		>::value),
		"Failed TEST"
		);
#else
	typedef decltype(gridtools::cache<IJ, fill>(arg0_t(), arg1_t(), arg2_t())) caches_ret_sequence_3_t;
	typedef decltype(gridtools::cache<IJK, fill>(arg0_t(), arg1_t())) caches_ret_sequence_2_t;
	typedef decltype(gridtools::cache<IJ, fill>(arg0_t())) caches_ret_sequence_1_t;

	static_assert((
		boost::mpl::equal<
		caches_ret_sequence_3_t,
		boost::mpl::vector3<detail::cache_impl<IJ, arg0_t, fill, USHRT_MAX, 0>, detail::cache_impl<IJ, arg1_t, fill, USHRT_MAX, 0>, detail::cache_impl<IJ, arg2_t, fill, USHRT_MAX, 0> >
		>::value),
		"Failed TEST"
		);
	static_assert((
		boost::mpl::equal<
		caches_ret_sequence_2_t,
		boost::mpl::vector2<detail::cache_impl<IJK, arg0_t, fill, USHRT_MAX, 0>, detail::cache_impl<IJK, arg1_t, fill, USHRT_MAX, 0> >
		>::value),
		"Failed TEST"
		);
	static_assert((
		boost::mpl::equal<
		caches_ret_sequence_1_t,
		boost::mpl::vector1<detail::cache_impl<IJ, arg0_t, fill, USHRT_MAX, 0> >
		>::value),
		"Failed TEST"
		);
#endif

    ASSERT_TRUE(true);
}
