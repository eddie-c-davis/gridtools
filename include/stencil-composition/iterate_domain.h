#pragma once
#include "iterate_domain_impl.h"

/**@file
   @brief file handling the access to the storage.
   This file implements some of the innermost data access operations of the library and thus it must be highly optimized.
   The naming convention used below distinguishes from the following two concepts:

   - a parameter: is a non-space dimension (e.g. time) such that derivatives are taken in the equations along this dimension.
   - a dimension: with an abuse of notation will denote any physical scalar field contained in the given storage, e.g. a velocity component, the pressure, or the energy. I.e. it is an extra dimension which can appear in the equations only derived in space or with respect to the parameter mentioned above.
   - a storage: is an instance of the storage class, and can contain one or more fields and dimensions. Every dimension consists of one or several snaphsots of the scalar fields
   (e.g. if the time T is the current dimension, 3 snapshots can be the fields at t, t+1, t+2)
   - a data snapshot: is a pointer to one single snapshot. The snapshots are arranged in the storages on a 1D array, regardless of the dimension and snapshot they refer to. The arg_type (or arg_decorator) class is
   responsible of computing the correct offests (relative to the given dimension) and address the storages correctly.

   The access to the storage is performed in the following steps:
   - the addresses of the first element of all the data fields in the storages involved in this stencil are saved in an array (m_storage_pointers)
   - the index of the storages is saved in another array (m_index)
   - when the functor gets called, the 'offsets' become visible (in the perfect worls they could possibly be known at compile time). In particular the index is moved to point to the correct address, and the correct data snapshot is selected.

   Graphical (ASCII) illustration:

   \verbatim
   ################## Storage #################
   #                ___________\              #
   #                  width    /              #
   #              | |*|*|*|*|*|*|    dim_1    #
   #   dimensions | |*|*|*|          dim_2    #
   #              v |*|*|*|*|*|      dim_3    #
   #                                          #
   #                 ^ ^ ^ ^ ^ ^              #
   #                 | | | | | |              #
   #                 snapshots                #
   #                                          #
   ################## Storage #################
   \endverbatim

*/

namespace gridtools {


    /**@brief class handling the computation of the */
    template <typename LocalDomain>
    struct iterate_domain {
        typedef typename boost::remove_pointer<
            typename boost::mpl::at_c<
                typename LocalDomain::mpl_storages, 0>::type
            >::type::value_type value_type;

        //typedef typename LocalDomain::local_args_type local_args_type;
        typedef typename LocalDomain::actual_args_type actual_args_type;
        //the number of storages  used in the current functor
        static const uint_t N_STORAGES=boost::mpl::size<actual_args_type>::value;
        //the total number of snapshot (one or several per storage)
        static const uint_t N_DATA_POINTERS=total_storages<
            actual_args_type,
            boost::mpl::size<typename LocalDomain::mpl_storages>::type::value-1 >::count;

private:
        // iterate_domain remembers the state. This is necessary when
        // we do finite differences and don't want to recompute all
        // the iterators (but simply use the ones available for the
        // current iteration storage for all the other storages)

        LocalDomain const& local_domain;
        array<uint_t,N_STORAGES> m_index;

        array<void* __restrict__, N_DATA_POINTERS>* __restrict__ m_data_pointer;

        strides_cached<N_STORAGES-1, typename LocalDomain::esf_args>* __restrict__ m_strides;

public:
        /**@brief constructor of the iterate_domain struct

           It assigns the storage pointers to the first elements of
           the data fields (for all the data_fields present in the
           current evaluation), and the indexes to access the data
           fields (one index per storage instance, so that one index
           might be shared among several data fileds)
        */
        GT_FUNCTION
        iterate_domain(LocalDomain const& local_domain)
            : local_domain(local_domain)
            ,m_data_pointer(0)
            ,m_strides(0)
            { }

        /** This functon set the addresses of the data values  before the computation
            begins.

            The EU stands for ExecutionUnit (thich may be a thread or a group of
            threasd. There are potentially two ids, one over i and one over j, since
            our execution model is parallel on (i,j). Defaulted to 1.
        */
        template<typename BackendType>
        GT_FUNCTION
        void assign_storage_pointers( array<void* __restrict__, N_DATA_POINTERS>* __restrict__ data_pointer ){

            const uint_t EU_id_i = BackendType::processing_element_i();
            const uint_t EU_id_j = BackendType::processing_element_j();
            m_data_pointer=data_pointer;
            assign_storage< N_STORAGES-1, BackendType >
                ::assign(*m_data_pointer, local_domain.local_args, EU_id_i, EU_id_j);
        }

        template<typename BackendType, typename Strides>
        GT_FUNCTION
        void assign_stride_pointers( Strides * __restrict__ strides_){
            m_strides=strides_;
            assign_strides< N_STORAGES-1, BackendType >::assign(*m_strides, local_domain.local_args);
        }


        /**@brief getter for the index array */
        GT_FUNCTION
        void get_index(array<int_t, N_STORAGES>& index) const
        {
            set_index_recur< N_STORAGES-1>::set(m_index, index);
        }

        /**@brief method for incrementing the index when moving forward along the k direction */
        template <typename Input>
        GT_FUNCTION
        void set_index(Input const& index)
        {
            set_index_recur< N_STORAGES-1>::set( index, m_index);
        }

        /**@brief method for incrementing the index when moving forward along the k direction */
        template <ushort_t Coordinate, enumtype::execution Execution>
        GT_FUNCTION
        void increment()
        {
            increment_index< N_STORAGES-1, Coordinate, Execution>::assign(local_domain.local_args, &m_index[0], *m_strides);
        }

        /**@brief method to set the first index in k (when iterating backwards or in the k-parallel case this can be different from zero)*/
        GT_FUNCTION
        void set_k_start(uint_t from)
        {
            increment_index<N_STORAGES-1, 2, enumtype::forward>::assign(local_domain.local_args, from, &m_index[0], *m_strides);
        }

        template <typename T>
        GT_FUNCTION
        void info(T const &x) const
        {
            local_domain.info(x);
        }


        /**@brief returns the value of the memory at the given address, plus the offset specified by the arg placeholder
           \param arg placeholder containing the storage ID and the offsets
           \param storage_pointer pointer to the first element of the specific data field used
        */
        template <typename ArgType, typename StoragePointer>
        GT_FUNCTION
        typename boost::mpl::at<typename LocalDomain::esf_args, typename ArgType::index_type>::type::value_type& __restrict__
        get_value(ArgType const& arg , StoragePointer & __restrict__ storage_pointer) const
{


#ifdef CXX11_ENABLED
            using storage_type = typename std::remove_reference<decltype(*boost::fusion::at<typename ArgType::index_type>(local_domain.local_args))>::type;
#else
                typedef typename boost::remove_reference<BOOST_TYPEOF( (*boost::fusion::at<typename ArgType::index_type>(local_domain.local_args)) )>::type storage_type;
#endif
                typename storage_type::value_type * __restrict__ real_storage_pointer=static_cast<typename storage_type::value_type*>(storage_pointer);

                //the following assert fails when an out of bound access is observed, i.e. either one of
                //i+offset_i or j+offset_j or k+offset_k is too large.
                //Most probably this is due to you specifying a positive offset which is larger than expected,
                //or maybe you did a mistake when specifying the ranges in the placehoders definition
                assert(boost::fusion::at<typename ArgType::index_type>(local_domain.local_args)->size() >  m_index[ArgType::index_type::value]
                       +(boost::fusion::at<typename ArgType::index_type>(local_domain.local_args))
                       ->_index(m_strides->template get<ArgType::index_type::value>(), arg)
                    );

                //the following assert fails when an out of bound access is observed,
                //i.e. when some offset is negative and either one of
                //i+offset_i or j+offset_j or k+offset_k is too small.
                //Most probably this is due to you specifying a negative offset which is
                //smaller than expected, or maybe you did a mistake when specifying the ranges
                //in the placehoders definition.
                // If you are running a parallel simulation another common reason for this to happen is
                // the definition of an halo region which is too small in one direction
                // std::cout<<"Storage Index: "<<ArgType::index_type::value<<" + "<<(boost::fusion::at<typename ArgType::index_type>(local_domain.local_args))->_index(arg.template n<ArgType::n_args>())<<std::endl;
        assert( (int_t)(m_index[ArgType::index_type::value])
               +(boost::fusion::at<typename ArgType::index_type>(local_domain.local_args))
                        ->_index(m_strides->template get<ArgType::index_type::value>(), arg)
                        >= 0);

#ifdef CXX11_ENABLED
                GRIDTOOLS_STATIC_ASSERT((gridtools::arg_decorator<ArgType>::n_args <= boost::mpl::at<typename LocalDomain::esf_args, typename ArgType::index_type>::type::storage_type::space_dimensions) <= gridtools::arg_decorator<ArgType>::n_dim, "access out of bound in the storage placeholder (arg_type). increase the number of dimensions when defining the placeholder.")
#endif
                return *(real_storage_pointer
                         +(m_index[ArgType::index_type::value])
                         +(boost::fusion::at<typename ArgType::index_type>(local_domain.local_args))
                         //here we suppose for the moment that ArgType::index_types are ordered like the LocalDomain::esf_args mpl vector
                         ->_index(m_strides->template get<ArgType::index_type::value>(), arg)
                    );
            }


        /**@brief local class instead of using the inline (cond)?a:b syntax, because in the latter both branches get compiled (generating sometimes a compile-time overflow) */
        template <bool condition, typename LocalD, typename ArgType>
        struct current_storage;

        template < typename LocalD, typename ArgType>
        struct current_storage<true, LocalD, ArgType>{
            static const uint_t value=0;
        };

        template < typename LocalD, typename ArgType>
        struct current_storage<false, LocalD, ArgType>{
            static const uint_t value=(total_storages< typename LocalD::local_args_type, ArgType::index_type::value-1 >::count);
        };


#ifdef CXX11_ENABLED
        /** @brief method called in the Do methods of the functors.
            specialization for the expr_direct_access<arg_type> placeholders
        */
        template <typename ArgType>
        GT_FUNCTION
        typename boost::mpl::at<typename LocalDomain::esf_args, typename ArgType::type::index_type>::type::value_type& __restrict__
        operator()(expr_direct_access<ArgType > const& arg) const {
            return get_value(arg, (*m_data_pointer)[current_storage<(ArgType::type::index_type::value==0), LocalDomain, typename ArgType::type >::value]);
        }
#endif

        /** @brief method called in the Do methods of the functors.
            specialization for the arg_type placeholders
        */
        template <typename ArgType>
        GT_FUNCTION
        typename boost::enable_if<typename boost::mpl::bool_<(ArgType::type::n_args <= boost::mpl::at<typename LocalDomain::esf_args, typename ArgType::type::index_type>::type::storage_type::space_dimensions)>::type,
                                                             typename boost::mpl::at<typename LocalDomain::esf_args, typename ArgType::type::index_type>::type::value_type
                                                             >::type& __restrict__
        operator()(ArgType const& arg) const {

            return get_value(arg, (*m_data_pointer)[current_storage<(ArgType::index_type::value==0), LocalDomain, typename ArgType::type >::value]);
        }


        /** @brief method called in the Do methods of the functors.
            Specialization for the arg_decorator placeholder (i.e. for extended storages, containg multiple snapshots of data fields with the same dimension and memory layout)*/
        template < typename ArgType>
        GT_FUNCTION
        typename boost::enable_if<
            typename boost::mpl::bool_<(ArgType::type::n_args > boost::mpl::at<typename LocalDomain::esf_args, typename ArgType::type::index_type>::type::storage_type::space_dimensions)>::type
                                  , typename boost::mpl::at<typename LocalDomain::esf_args, typename ArgType::type::index_type>::type::value_type >::type&  __restrict__
        operator()(ArgType const& arg) const {

#ifdef CXX11_ENABLED
            using storage_type = typename std::remove_reference<decltype(*boost::fusion::at<typename ArgType::type::index_type>(local_domain.local_args))>::type;
#else
            typedef typename boost::remove_reference<BOOST_TYPEOF( (*boost::fusion::at<typename ArgType::type::index_type>(local_domain.local_args)) )>::type storage_type;
#endif
            //if the following assertion fails you have specified a dimension for the extended storage
            //which does not correspond to the size of the extended placeholder for that storage
            /* BOOST_STATIC_ASSERT(storage_type::n_fields==ArgType::type::n_args); */

            //for the moment the extra dimensionality of the storage is limited to max 2
            //(3 space dim + 2 extra= 5, which gives n_args==4)
            GRIDTOOLS_STATIC_ASSERT(N_DATA_POINTERS>0, "the total number of snapshots must be larger than 0 in each functor")
                GRIDTOOLS_STATIC_ASSERT(ArgType::type::n_args <= ArgType::type::n_dim, "access out of bound in the storage placeholder (arg_type). increase the number of dimensions when defining the placeholder.")


                GRIDTOOLS_STATIC_ASSERT((storage_type::traits::n_fields%storage_type::traits::n_width==0), "You specified a non-rectangular field: in the pre-C++11 version of the library only fields with the same number of snapshots in each field dimension are allowed.")

                // std::cout<<" offsets: "<<arg.template get<0>()<<" , "<<arg.template get<1>()<<" , "<<arg.template get<2>()<<" , "<<std::endl;

                return get_value(arg,
                                 (*m_data_pointer)[
                                     storage_type::get_index
                                     (
                                         (
                                             ArgType::type::n_args <= storage_type::space_dimensions+1 ? // static if
                                             arg.template get<0>() //offset for the current dimension
                                             :
                                             arg.template get<0>() //offset for the current dimension
                                             //limitation to "rectangular" vector fields for non-static fields dimensions
                                             +  arg.template get<1>()
                                             * storage_type::traits::n_width  //stride of the current dimension inside the vector of storages
                                             ))//+ the offset of the other extra dimension
                                     + current_storage<(ArgType::type::index_type::value==0), LocalDomain, typename ArgType::type>::value
                                     ]);
        }



#if defined(CXX11_ENABLED) && !defined(__CUDACC__)

        /** @brief method called in the Do methods of the functors.

            Specialization for the arg_decorator placeholder (i.e. for extended storages, containg multiple snapshots of data fields with the same dimension and memory layout)*/
        template < typename ArgType, typename ... Pairs>
        GT_FUNCTION
        typename boost::mpl::at<typename LocalDomain::esf_args, typename ArgType::index_type>::type::value_type& __restrict__
        operator()(arg_mixed<ArgType, Pairs ... > const& arg) const {

            typedef arg_mixed<ArgType, Pairs ... > arg_mixed_t;
            using storage_type = typename std::remove_reference<decltype(*boost::fusion::at<typename ArgType::type::index_type>(local_domain.local_args))>::type;

            //if the following assertion fails you have specified a dimension for the extended storage
            //which does not correspond to the size of the extended placeholder for that storage
            /* BOOST_STATIC_ASSERT(storage_type::n_fields==ArgType::n_args); */

            //for the moment the extra dimensionality of the storage is limited to max 2
            //(3 space dim + 2 extra= 5, which gives n_args==4)
            GRIDTOOLS_STATIC_ASSERT(N_DATA_POINTERS>0, "the total number of snapshots must be larger than 0 in each functor")
                GRIDTOOLS_STATIC_ASSERT(ArgType::type::n_args <= ArgType::type::n_dim, "access out of bound in the storage placeholder (arg_type). increase the number of dimensions when defining the placeholder.")


                GRIDTOOLS_STATIC_ASSERT((storage_type::traits::n_fields%storage_type::traits::n_width==0), "You specified a non-rectangular field: in the pre-C++11 version of the library only fields with the same number of snapshots in each field dimension are allowed.")

                return get_value(arg,
                                 (*m_data_pointer)[ //static if
                                     //TODO: re implement offsets in arg_type which can be or not constexpr (not in a vector)
                                     storage_type::get_index(
                                         (
                                             ArgType::type::n_args <= storage_type::space_dimensions+1 ? // static if
                                             arg_mixed_t::template get_constexpr<0>() //offset for the current dimension
                                             :
                                             arg_mixed_t::template get_constexpr<0>() //offset for the current dimension
 #ifdef CXX11_ENABLED
                                             //hypotheses (we can weaken it using constexpr static functions):
                                             //storage offsets are known at compile-time
                                             + compute_storage_offset< typename storage_type::traits, arg_mixed_t::template get_constexpr<1>(), storage_type::traits::n_dimensions-1 >::value //stride of the current dimension inside the vector of storages
#else
                                             //limitation to "rectangular" vector fields for non-C++11 storages
                                             +  arg_mixed_t::template get_constexpr<1>()
                                             * storage_type::traits::n_width  //stride of the current dimension inside the vector of storages
#endif
                                             ))//+ the offset of the other extra dimension
                                     + current_storage<(ArgType::type::index_type::value==0), LocalDomain, typename ArgType::type>::value
                                     ]);

        }

        /** @brief method called in the Do methods of the functors.

            specialization for the expr_direct_access<ArgType> placeholders (high level syntax: '@plch').
            Allows direct access to the storage by only using the offsets
        */
        template <typename ArgType, typename StoragePointer>
        GT_FUNCTION
        typename boost::mpl::at<typename LocalDomain::esf_args, typename ArgType::index_type>::type::value_type& __restrict__
        get_value (expr_direct_access<ArgType> const& arg, StoragePointer & __restrict__ storage_pointer) const {

            assert(boost::fusion::at<typename ArgType::index_type>(local_domain.local_args)->size() >  (boost::fusion::at<typename ArgType::index_type>(local_domain.local_args))
                   ->_index(m_strides->template get<ArgType::index_type::value>(), arg.first_operand));

         assert((boost::fusion::at<typename ArgType::index_type>(local_domain.local_args))
                ->_index(m_strides->template get<ArgType::index_type::value>(), arg.first_operand) >= 0);
            GRIDTOOLS_STATIC_ASSERT((gridtools::arg_decorator<ArgType>::n_args <= boost::mpl::at<typename LocalDomain::esf_args, typename ArgType::index_type>::type::storage_type::space_dimensions) <= gridtools::arg_decorator<ArgType>::n_dim, "access out of bound in the storage placeholder (arg_type). increase the number of dimensions when defining the placeholder.")

#ifdef CXX11_ENABLED
                using storage_type = typename std::remove_reference<decltype(*boost::fusion::at<typename ArgType::index_type>(local_domain.local_args))>::type;
#else
            typedef typename boost::remove_reference<BOOST_TYPEOF( (*boost::fusion::at<typename ArgType::index_type>(local_domain.local_args)) )>::type storage_type;
#endif

            typename storage_type::value_type * __restrict__ real_storage_pointer=static_cast<typename storage_type::value_type*>(storage_pointer);

            return *(real_storage_pointer
                     +(boost::fusion::at<typename ArgType::index_type>(local_domain.local_args))
                     ->_index(m_strides->template get<ArgType::index_type::value>(), arg.first_operand));
        }

        /**\section binding_expressions (Expressions Bindings)
           @brief these functions get called by the operator () in gridtools::iterate_domain, i.e. in the functor Do method defined at the application level
           They evalueate the operator passed as argument, by recursively evaluating its arguments
           @{
        */

        /** plus evaluation*/
        template <typename ArgType1, typename ArgType2>
        GT_FUNCTION
        auto value(expr_plus<ArgType1, ArgType2> const& arg) const -> decltype((*this)(arg.first_operand) + (*this)(arg.second_operand)) {return (*this)(arg.first_operand) + (*this)(arg.second_operand);}

        /** minus evaluation*/
        template <typename ArgType1, typename ArgType2>
        GT_FUNCTION
        auto value(expr_minus<ArgType1, ArgType2> const& arg) const -> decltype((*this)(arg.first_operand) - (*this)(arg.second_operand)) {return (*this)(arg.first_operand) - (*this)(arg.second_operand);}

        /** multiplication evaluation*/
        template <typename ArgType1, typename ArgType2>
        GT_FUNCTION
        auto value(expr_times<ArgType1, ArgType2> const& arg) const -> decltype((*this)(arg.first_operand) * (*this)(arg.second_operand)) {return (*this)(arg.first_operand) * (*this)(arg.second_operand);}

        /** division evaluation*/
        template <typename ArgType1, typename ArgType2>
        GT_FUNCTION
        auto value(expr_divide<ArgType1, ArgType2> const& arg) const -> decltype((*this)(arg.first_operand) / (*this)(arg.second_operand)) {return (*this)(arg.first_operand) / (*this)(arg.second_operand);}

        /**\subsection specialization (Partial Specializations)
           partial specializations for double (or float)
           @{*/
        /** sum with scalar evaluation*/
        template <typename ArgType1, typename FloatType, typename boost::enable_if<typename boost::is_floating_point<FloatType>::type, int >::type=0 >
        GT_FUNCTION
        auto value_scalar(expr_plus<ArgType1, FloatType> const& arg) const -> decltype((*this)(arg.first_operand) + arg.second_operand) {return (*this)(arg.first_operand) + arg.second_operand;}

        /** subtract with scalar evaluation*/
        template <typename ArgType1, typename FloatType, typename boost::enable_if<typename boost::is_floating_point<FloatType>::type, int >::type=0 >
        GT_FUNCTION
        auto value_scalar(expr_minus<ArgType1, FloatType> const& arg) const -> decltype((*this)(arg.first_operand) - arg.second_operand) {return (*this)(arg.first_operand) - arg.second_operand;}

        /** multiply with scalar evaluation*/
        template <typename ArgType1, typename FloatType, typename boost::enable_if<typename boost::is_floating_point<FloatType>::type, int >::type=0 >
        GT_FUNCTION
        auto value_scalar(expr_times<ArgType1, FloatType> const& arg) const -> decltype((*this)(arg.first_operand) * arg.second_operand) {return (*this)(arg.first_operand) * arg.second_operand;}

        /** divide with scalar evaluation*/
        template <typename ArgType1, typename FloatType, typename boost::enable_if<typename boost::is_floating_point<FloatType>::type, int >::type=0 >
        GT_FUNCTION
        auto value_scalar(expr_divide<ArgType1, FloatType> const& arg) const -> decltype((*this)(arg.first_operand) / arg.second_operand) {return (*this)(arg.first_operand) / arg.second_operand;}

#ifndef __CUDACC__
        /** power of scalar evaluation*/
        template <typename FloatType, typename IntType, typename boost::enable_if<typename boost::is_floating_point<FloatType>::type, int >::type=0 , typename boost::enable_if<typename boost::is_integral<IntType>::type, int >::type=0 >
        GT_FUNCTION
        auto value_scalar(expr_exp<FloatType, IntType> const& arg) const -> decltype(std::pow (arg.first_operand,  arg.second_operand)) {return std::pow(arg.first_operand, arg.second_operand);}

#else //ifndef __CUDACC__
        /** power of scalar evaluation of CUDA*/
        template <typename FloatType, typename IntType, typename boost::enable_if<typename boost::is_floating_point<FloatType>::type, int >::type=0 , typename boost::enable_if<typename boost::is_integral<IntType>::type, int >::type=0 >
        GT_FUNCTION
        auto value_scalar(expr_exp<FloatType, IntType> const& arg) const -> decltype(std::pow (arg.first_operand,  arg.second_operand)) {return products<2>::apply(arg.first_operand);}

#endif //ifndef __CUDACC__

        /**
           @}
           \subsection specialization2 (Partial Specializations)
           @brief partial specializations for integer
           Here we do not use the typedef int_t, because otherwise the interface would be polluted with casting
           (the user would have to cast all the literal types (-1, 0, 1, 2 .... ) to int_t before using them in the expression)
           @{*/

        template <typename ArgType1, typename IntType, typename boost::enable_if<typename boost::is_integral<IntType>::type, int >::type=0 >
        GT_FUNCTION
        auto value_int(expr_exp<ArgType1, IntType> const& arg) const -> decltype(products<2>::apply((*this)(arg.first_operand))) {return products<2>::apply((*this)(arg.first_operand));
        }

        template <typename ArgType1, /*typename IntType, IntType*/int exponent/*, typename boost::enable_if<typename boost::is_integral<IntType>::type, int >::type=0 */>
            GT_FUNCTION
            auto value_int(expr_pow<ArgType1, exponent> const& arg) const -> decltype(products<exponent>::apply((*this)(arg.first_operand))) {return products<exponent>::apply((*this)(arg.first_operand));}

        /**@}@}*/

        /** @brief method called in the Do methods of the functors. */
        template <typename FirstArg, typename SecondArg, template<typename Arg1, typename Arg2> class Expression >
        GT_FUNCTION
        auto operator() (Expression<FirstArg, SecondArg> const& arg) const ->decltype(this->value(arg)) {
            //arg.to_string();
            return value(arg);
        }

        /** @brief method called in the Do methods of the functors.
            partial specializations for double (or float)*/
        template <typename Arg, template<typename Arg1, typename Arg2> class Expression, typename FloatType, typename boost::enable_if<typename boost::is_floating_point<FloatType>::type, int >::type=0 >
        GT_FUNCTION
        auto operator() (Expression<Arg, FloatType> const& arg) const ->decltype(this->value_scalar(arg)) {
            return value_scalar(arg);
        }

        /** @brief method called in the Do methods of the functors.
            partial specializations for int. Here we do not use the typedef int_t, because otherwise the interface would be polluted with casting
            (the user would have to cast all the numbers (-1, 0, 1, 2 .... ) to int_t before using them in the expression)*/
        // template <typename Arg, int Arg2, template<typename Arg1, int a> class Expression >
        template <typename Arg, template<typename Arg1, typename Arg2> class Expression, typename IntType, typename boost::enable_if<typename boost::is_integral<IntType>::type, int >::type=0 >
        GT_FUNCTION
        auto operator() (Expression<Arg, IntType> const& arg) const ->decltype(this->value_int(arg)) {
            return value_int(arg);
        }

        template <typename Arg, template<typename Arg1, int Arg2> class Expression, /*typename IntType, typename boost::enable_if<typename boost::is_integral<IntType>::type, int >::type=0*/int exponent >
        GT_FUNCTION
        auto operator() (Expression<Arg, exponent> const& arg) const ->decltype(this->value_int(arg)) {
            return value_int(arg);
        }

#endif //CXX11_ENABLED

        /**@brief method for incrementing the index when moving forward along the k direction */
        template <ushort_t Coordinate>
        GT_FUNCTION
        void initialize(uint_t const& index=0, uint_t const& block=0)
        {
            initialize_index< N_STORAGES-1, Coordinate>::assign(local_domain.local_args, index, block, &m_index[0], *m_strides);
        }


    };

    /**@brief class handling the computation of the */
    template <typename LocalDomain>
    struct stateful_iterate_domain: public iterate_domain<LocalDomain> {

        typedef iterate_domain<LocalDomain> base_type;

        uint_t i,j,k;

#ifdef CXX11_ENABLED
        using iterate_domain<LocalDomain>::iterate_domain;
#else
        GT_FUNCTION
        stateful_iterate_domain(LocalDomain const& local_domain)
            : iterate_domain<LocalDomain>(local_domain)
        {}
#endif
        /**@brief method for incrementing the index when moving forward along the k direction */
        template <ushort_t Coordinate, enumtype::execution Execution>
        GT_FUNCTION
        void increment()
        {
            if (Coordinate==0) {
                i++;
            }
            if (Coordinate==1) {
                j++;
            }

            base_type::template increment<Coordinate, Execution>();
        }
            /**@brief method to set the first index in k (when iterating backwards or in the k-parallel case this can be different from zero)*/
        GT_FUNCTION
        void set_k_start(uint_t from)
        {
            k = from;
            base_type::set_k_start(from);
        }

    };

} // namespace gridtools
