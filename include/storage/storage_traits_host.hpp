/*
   Copyright 2016 GridTools Consortium

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#pragma once
#include "../common/defs.hpp"
#include "storage.hpp"
#include "base_storage.hpp"
#include "meta_storage.hpp"
#include "meta_storage_aligned.hpp"
#include "meta_storage_base.hpp"

namespace gridtools {
    template < enumtype::platform T >
    struct storage_traits_from_id;

    /**Traits struct, containing the types which are specific for the host backend*/
    template <>
    struct storage_traits_from_id< enumtype::Host > {

        /**
           @brief pointer type associated to the host backend
         */
        template < typename T >
        struct pointer {
            typedef wrap_pointer< T, true > type;
        };

        /**
           @brief storage type associated to the host backend
         */
        template < typename ValueType, typename MetaData, short_t FieldDim = 1 >
        struct select_storage {
            GRIDTOOLS_STATIC_ASSERT((is_meta_storage< MetaData >::value), "wrong type for the storage_info");
            typedef storage< base_storage< typename pointer< ValueType >::type, MetaData, FieldDim > > type;
        };

        struct default_alignment {
            typedef aligned< 0 > type;
        };

        /**
           @brief storage info type associated to the host backend
         */
        template < typename IndexType, typename Layout, bool Temp, typename Halo, typename Alignment >
        struct select_meta_storage {
            GRIDTOOLS_STATIC_ASSERT((is_layout_map< Layout >::value), "wrong type for the storage_info");
            GRIDTOOLS_STATIC_ASSERT(is_halo< Halo >::type::value, "wrong type");
            GRIDTOOLS_STATIC_ASSERT(is_aligned< Alignment >::type::value, "wrong type");

            typedef meta_storage<
                meta_storage_aligned< meta_storage_base< IndexType::value, Layout, Temp >, Alignment, Halo > > type;
        };
    };
}
