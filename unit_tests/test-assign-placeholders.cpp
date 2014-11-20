
#include <gridtools.h>

#ifdef CUDA_EXAMPLE
#include <stencil-composition/backend_cuda.h>
#else
#include <stencil-composition/backend_host.h>
#endif

#include <boost/fusion/include/make_vector.hpp>

/*
  This file shows an implementation of the "horizontal diffusion" stencil, similar to the one used in COSMO
 */

using namespace gridtools;
using namespace enumtype;


bool assign_placeholders() {

#ifdef CUDA_EXAMPLE
#define BACKEND backend<Cuda, Naive >
#else
#ifdef BACKEND_BLOCK
#define BACKEND backend<Host, Block >
#else
#define BACKEND backend<Host, Naive >
#endif
#endif

    //    typedef gridtools::STORAGE<double, gridtools::layout_map<0,1,2> > storage_type;

    typedef gridtools::BACKEND::storage_type<float_type, gridtools::layout_map<0,1,2> >::type storage_type;
    typedef gridtools::BACKEND::temporary_storage_type<float_type, gridtools::layout_map<0,1,2> >::type tmp_storage_type;

    uint_t d1=5;
    uint_t d2=5;
    uint_t d3=5;

    storage_type in(d1,d2,d3,-1, std::string("in"));
    storage_type out(d1,d2,d3,-7.3, std::string("out"));
    storage_type coeff(d1,d2,d3,8, std::string("coeff"));


    // Definition of placeholders. The order of them reflect the order the user will deal with them
    // especially the non-temporary ones, in the construction of the domain
    typedef arg<0, tmp_storage_type > p_lap;
    typedef arg<1, tmp_storage_type > p_flx;
    typedef arg<2, tmp_storage_type > p_fly;
    typedef arg<3, storage_type > p_coeff;
    typedef arg<4, storage_type > p_in;
    typedef arg<5, storage_type > p_out;

    // An array of placeholders to be passed to the domain
    // I'm using mpl::vector, but the final API should look slightly simpler
    typedef boost::mpl::vector<p_lap, p_flx, p_fly, p_coeff, p_in, p_out> arg_type_list;

    // printf("coeff (3) pointer: %x\n", &coeff);
    // printf("in    (4) pointer: %x\n", &in);
    // printf("out   (5) pointer: %x\n", &out);

    gridtools::domain_type<arg_type_list> domain( (p_out() = out), (p_in() = in), (p_coeff() = coeff) );

    return ((boost::fusion::at_c<3>(domain.storage_pointers) == &coeff) &&
            (boost::fusion::at_c<4>(domain.storage_pointers) == &in) &&
            (boost::fusion::at_c<5>(domain.storage_pointers) == &out));
}
