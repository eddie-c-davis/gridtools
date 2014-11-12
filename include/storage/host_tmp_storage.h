#pragma once

#include "base_storage.h"
#include <boost/mpl/int.hpp>

namespace gridtools {


    template < enumtype::backend Backend
               , typename ValueType
               , typename Layout
               , uint_t TileI
               , uint_t TileJ
               , uint_t MinusI
               , uint_t MinusJ
               , uint_t PlusI
               , uint_t PlusJ
               >
    struct host_tmp_storage : public base_storage<Backend
                                                  , ValueType
                                                  , Layout
                                                  , true
                                                  >
    {

        typedef base_storage<Backend
                             , ValueType
                             , Layout
                             , true> base_type;


        typedef typename base_type::layout layout;
        typedef typename base_type::value_type value_type;
        typedef typename base_type::iterator_type  iterator_type;
        typedef typename base_type::const_iterator_type const_iterator_type;

        typedef static_int<MinusI> minusi;
        typedef static_int<MinusJ> minusj;
        typedef static_int<PlusI> plusi;
        typedef static_int<PlusJ> plusj;

        // using base_type::m_dims;
        using base_type::m_strides;
        // using base_type::m_size;
        using base_type::is_set;

        static const std::string info_string;

        // int m_tile[3];
        uint_t m_halo[3];
        uint_t m_initial_offsets[3];

        // std::string m_name;

        // value_type* data;

        explicit host_tmp_storage(uint_t initial_offset_i,
                                  uint_t initial_offset_j,
                                  uint_t dim3,
                                  //int initial_offset_k=0,
                                  value_type init = value_type(),
                                  std::string const& s = std::string("default name") )
            : base_type(TileI+MinusI+PlusI,TileJ+MinusJ+PlusJ, dim3, init, s)
        {
            m_halo[0]=MinusI;
            m_halo[1]=MinusJ;
            m_halo[2]=0;
            m_initial_offsets[0] = initial_offset_i;
            m_initial_offsets[1] = initial_offset_j;
            m_initial_offsets[2] = 0 /* initial_offset_k*/;
        }


        host_tmp_storage() {}

        // ~host_tmp_storage() {
        //     if (is_set) {
        //         //std::cout << "deleting " << std::hex << data << std::endl;
        //         delete[] m_data;
        //     }
        // }

        virtual void info() const {
            // std::cout << "Temporary storage "
            //           << m_dims[0] << "x"
            //           << m_dims[1] << "x"
            //           << m_dims[2] << ", "
            //           << m_halo[0] << "x"
            //           << m_halo[1] << "x"
            //           << m_halo[2] << ", "
            //           << this->m_name
            //           << std::endl;
        }

        iterator_type move_to(uint_t i,uint_t j,uint_t k) const {
            return const_cast<iterator_type>(&(base_type::m_data[_index(i,j,k)]));
            //return &(base_type::m_data[_index(i,j,k)]);
        }

        GT_FUNCTION
        value_type& operator()(uint_t i, uint_t j, uint_t k) {
            return base_type::m_data[_index(i,j,k)];
        }


        GT_FUNCTION
        value_type const & operator()(uint_t i, uint_t j, uint_t k) const {
            return base_type::m_data[_index(i,j,k)];
        }


        uint_t _index(uint_t i, uint_t j, uint_t k) const {
            uint_t index;
            // std::cout << "                                                  index "
            //           << "m_dims_i "
            //           << m_dims[0]
            //           << " "
            //           << "m_dims_j "
            //           << m_dims[1]
            //           << " "
            //           << "m_dims_k "
            //           << m_dims[2]
            //           << " - "
            //           << "m_halo_i "
            //           << m_halo[0]
            //           << " "
            //           << "m_halo_j "
            //           << m_halo[1]
            //           << " "
            //           << "m_halo_k "
            //           << m_halo[2]
            //           << " - "
            //           << "i "
            //           << i
            //           << " "
            //           << "j "
            //           << j
            //           << " "
            //           << "k "
            //           << k
            //           << std::endl;
            // info();

            uint_t _i = ((layout::template find<0>(i,j,k)) - layout::template find<0>(m_initial_offsets) + layout::template find<0>(m_halo));
            std::cout << "uint_t _i = ((" << layout::template find<0>(i,j,k) << ")-" << layout::template find<0>(m_initial_offsets) << "+" << layout::template find<0>(m_halo) << ")" << std::endl;
            uint_t _j = ((layout::template find<1>(i,j,k)) - layout::template find<1>(m_initial_offsets) + layout::template find<1>(m_halo));
            std::cout << "uint_t _j = ((" << layout::template find<1>(i,j,k) << ")-" << layout::template find<1>(m_initial_offsets) << "+" << layout::template find<1>(m_halo) << ")" << std::endl;
            uint_t _k = ((layout::template find<2>(i,j,k)) - layout::template find<2>(m_initial_offsets) + layout::template find<2>(m_halo));

            index =
                /*layout::template find<2>(m_dims) * layout::template find<1>(m_dims)*/m_strides[1] * _i +
                /*layout::template find<2>(m_dims)*/m_strides[2] * _j + _k;



            // std::cout << " i  = " << _i
            //           << " j  = " << _j
            //           << " k  = " << _k
            //           << "   = " << index
            //           << std::endl;

            assert(index >= 0);
            assert(index <m_strides[0]);

            return index;
        }

        template <uint_t Coordinate>
        GT_FUNCTION
        void increment(uint_t& b, uint_t* index){
            *index += base_type::template strides<Coordinate>(m_strides)-b*(Coordinate==0?TileI:TileJ);
        }

        template <uint_t Coordinate>
        GT_FUNCTION
        void decrement(uint_t& b, uint_t* index){
            *index -= base_type::template strides<Coordinate>(m_strides)-b*(Coordinate==0?TileI:TileJ);
        }

        template <uint_t Coordinate>
        GT_FUNCTION
        void increment(uint_t& dimension, uint_t& b, uint_t* index){
            *index += base_type::template strides<Coordinate>(m_strides)*dimension-b*(Coordinate==0?TileI:TileJ);
        }

        template <uint_t Coordinate>
        GT_FUNCTION
        void decrement(uint_t& dimension, uint_t& b, uint_t* index){
            *index-= base_type::template strides<Coordinate>(m_strides)*dimension-b*(Coordinate==0?TileI:TileJ);
        }


    };



//huge waste of space because the C++ standard doesn't want me to initialize static const inline
    template < enumtype::backend Backend, typename ValueType, typename Layout, uint_t TileI, uint_t TileJ, uint_t MinusI, uint_t MinusJ, uint_t PlusI, uint_t PlusJ
               >
    const std::string host_tmp_storage<Backend, ValueType, Layout, TileI, TileJ, MinusI, MinusJ, PlusI, PlusJ>
    ::info_string=boost::lexical_cast<std::string>(minusi::value)+
                                             boost::lexical_cast<std::string>(minusj::value)+
                                             boost::lexical_cast<std::string>(plusi::value)+
                                             boost::lexical_cast<std::string>(plusj::value);

    template <enumtype::backend Backend,
              typename ValueType
              , typename Layout
              , uint_t TileI
              , uint_t TileJ
              , uint_t MinusI
              , uint_t MinusJ
              , uint_t PlusI
              , uint_t PlusJ
              >
    std::ostream& operator<<(std::ostream& s,
                             host_tmp_storage<
                              Backend
                             , ValueType
                             , Layout
                             , TileI
                             , TileJ
                             , MinusI
                             , MinusJ
                             , PlusI
                             , PlusJ
                             > const & x) {
        return s << "host_tmp_storage<...,"
                 << TileI << ", "
                 << TileJ << ", "
                 << MinusI << ", "
                 << MinusJ << ", "
                 << PlusI << ", "
                 << PlusJ << "> ";
    }


    template <enumtype::backend Backend
              , typename ValueType
              , typename Layout
              , uint_t TileI
              , uint_t TileJ
              , uint_t MinusI
              , uint_t MinusJ
              , uint_t PlusI
               , uint_t PlusJ
              >
    struct is_storage<host_tmp_storage<
                          Backend
                          , ValueType
                          , Layout
                          , TileI
                          , TileJ
                          , MinusI
                          , MinusJ
                          , PlusI
                          , PlusJ
                          >* >
      : boost::false_type
    {};


    template <enumtype::backend Backend
              , typename ValueType
              , typename Layout
              , uint_t TileI
              , uint_t TileJ
              , uint_t MinusI
              , uint_t MinusJ
              , uint_t PlusI
               , uint_t PlusJ
              >
    struct is_temporary_storage<host_tmp_storage<
                                    Backend
                                    , ValueType
                                    , Layout
                                    , TileI
                                    , TileJ
                                    , MinusI
                                    , MinusJ
                                    , PlusI
                                    , PlusJ
                                    >*& >
      : boost::true_type
    {};

    template <enumtype::backend Backend
              , typename ValueType
              , typename Layout
              , uint_t TileI
              , uint_t TileJ
              , uint_t MinusI
              , uint_t MinusJ
              , uint_t PlusI
              , uint_t PlusJ
              >
    struct is_temporary_storage<host_tmp_storage<
                                    Backend,
                                    ValueType
                                    , Layout
                                    , TileI
                                    , TileJ
                                    , MinusI
                                    , MinusJ
                                    , PlusI
                                    , PlusJ
                                    >* >
      : boost::true_type
    {};

    template <enumtype::backend Backend
              , typename ValueType
              , typename Layout
              , uint_t TileI
              , uint_t TileJ
              , uint_t MinusI
              , uint_t MinusJ
              , uint_t PlusI
              , uint_t PlusJ
              >
    struct is_temporary_storage<host_tmp_storage<
                                    Backend
                                    , ValueType
                                    , Layout
                                    , TileI
                                    , TileJ
                                    , MinusI
                                    , MinusJ
                                    , PlusI
                                    , PlusJ
                                    > &>
    : boost::true_type
    {};

    template <enumtype::backend Backend
              , typename ValueType
              , typename Layout
              , uint_t TileI
              , uint_t TileJ
              , uint_t MinusI
              , uint_t MinusJ
              , uint_t PlusI
              , uint_t PlusJ
              >
    struct is_temporary_storage<host_tmp_storage<
                                    Backend
                                    , ValueType
                                    , Layout
                                    , TileI
                                    , TileJ
                                    , MinusI
                                    , MinusJ
                                    , PlusI
                                    , PlusJ
                                    > const& >
    : boost::true_type
    {};

} // namespace gridtools
