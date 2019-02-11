#pragma once

#include "vert_db_config.h"
#include "vert_db_types.h"

#define VERTDB_DEF_ASSIGN(item_def, field, value) \
{                                                 \
    item_def.##field = value;                     \
    item_def.flags |= vd::k_item_##field;         \
}

#define VERTDB_DEF_SETTER(field)                  \
template<typename T>                              \
void set_##field(const T &field)                  \
{                                                 \
    VERTDB_DEF_ASSIGN((*this), field, field);     \
};

#define VERTDB_DEF_HAS(field)                     \
bool has_##field() const                          \
{                                                 \
    return flag_is_set( flags, k_item_##field );  \
}

#define VERTDB_DEF_APPLYER(field)                          \
template<typename S>                                       \
bool apply_##field( VERTDB_VERTID key, S &storage ) const  \
{                                                          \
    if( has_##field() )                                    \
    {                                                      \
        storage[key] = this->##field;                      \
        return true;                                       \
    }                                                      \
    return false;                                          \
}

#define VERTDB_DEF_GATHER(field)                           \
template<typename S>                                       \
bool gather_##field( VERTDB_VERTID key, const S &storage ) \
{                                                          \
    auto found = storage.find(key);                        \
    if( found == storage.end() )                           \
        return false;                                      \
    this->set_##field( found->second);                     \
    return true;                                           \
}

#define VERTDB_DEF_ASSIGNER(field)                         \
bool assign_##field( const self_type &other ) const        \
{                                                          \
    if( other.has_##field() )                              \
    {                                                      \
        this->set_##field(other.##field);                  \
        return true;                                       \
    }                                                      \
    return false;                                          \
}


#define VERTDB_DEF_ACCESSORS(field) \
VERTDB_DEF_SETTER(field)            \
VERTDB_DEF_HAS(field)               \
VERTDB_DEF_APPLYER(field)           \
VERTDB_DEF_GATHER(field)            \
VERTDB_DEF_ASSIGNER(field)

namespace vd
{
    typedef int item_flags_backing;
    enum item_flags : item_flags_backing
    {
        k_item_none = 0,
        k_item_id = 1 << 0,
        k_item_position = 1 << 1,
        k_item_normal = 1 << 2,
        k_item_uvw = 1 << 3,
        k_item_weights = 1 << 4,
        k_item_connects = 1 << 5,
        k_item_user_data = 1 << 6,
        k_item_last = k_item_user_data,
        k_item_all = ( k_item_last << 1 ) - 1,
    };

    inline item_flags operator| ( item_flags a, item_flags b )
    {
        return static_cast<item_flags>( static_cast<item_flags_backing>( a ) | static_cast<item_flags_backing>( b ) );
    }

    inline item_flags& operator|= ( item_flags &a, item_flags b )
    {
        a = a | b;
        return a;
    }

    inline item_flags operator& ( item_flags a, item_flags b )
    {
        return static_cast<item_flags>( static_cast<item_flags_backing>( a ) & static_cast<item_flags_backing>( b ) );
    }

    inline item_flags& operator&= ( item_flags &a, item_flags b )
    {
        a = a & b;
        return a;
    }

    inline bool flag_is_set( item_flags flags, item_flags check )
    {
        return ( flags & check ) != 0;
    }

    inline item_flags flag_without( item_flags flags, item_flags remove )
    {
        item_flags result = flags;
        result &= static_cast<item_flags>(~remove);
        return result;
    }

    template<typename T>
    struct db_item_def
    {
        typedef db_item_def<T> self_type;

        item_flags flags{};

        vert_id id{};
        vec3 position{};
        vec3 normal{};
        vec3 uvw{};
        bone_weights weights{};
        vert_connects connects{};

        T user_data{};

        VERTDB_DEF_ACCESSORS( id )
        VERTDB_DEF_ACCESSORS( position )
        VERTDB_DEF_ACCESSORS( normal )
        VERTDB_DEF_ACCESSORS( uvw )
        VERTDB_DEF_ACCESSORS( weights )
        VERTDB_DEF_ACCESSORS( connects )
        VERTDB_DEF_ACCESSORS( user_data )

        void assign( const db_item_def<T> &other )
        {
            assign_id( id );
            assign_position( position );
            assign_normal( normal );
            assign_uvw( uvw );
            assign_weights( weights );
            assign_connects( other.connects );
            assign_user_data( user_data );
        }
    };

    struct weight_finder
    {
        weight_finder( const bone_weight::first_type &weight )
            : m_weight( weight )
        {
        }

        bool operator()( const bone_weight &compare )
        {
            return m_weight == compare.first;
        }

        bone_weight::first_type m_weight;
    };

    inline bone_weights combine_weights( const bone_weights a, const bone_weights b, real modifier=1 )
    {
        bone_weights results = a;

        for( const bone_weight &weight_pair : b )
        {
            weight_finder finder( weight_pair.first );
            auto &found = find_if( results.begin(), results.end(), finder );
            if( found == results.end() )
            {
                results.emplace_back( weight_pair );
            }
            else
            {
                found->second += weight_pair.second * modifier;
            }
        }

        return results;
    }

    template<typename T>
    vert_connects combine_connects( const db_item_def<T> &a, const db_item_def<T> &b )
    {
        vert_connects result;

        for( const auto &connect : a.connects )
        {
            if( (connect != a.id) && (connect != b.id) )
            {
                result.emplace_back( connect );
            }
        }

        for( const auto &connect : b.connects )
        {
            if( (connect != a.id) && (connect != b.id) )
            {
                result.emplace_back( connect );
            }
        }

        return result;
    }

    template<typename T>
    inline db_item_def<T> operator+( const db_item_def<T> &a, const db_item_def<T> &b )
    {
        db_item_def<T> result{};

        result.set_position( a.position + b.position );
        results.set_normal( a.normal + b.normal );
        result.set_uvw( a.uvw + b.uvw );

        result.set_weights( combine_weights( a.weights, b.weights ) );
        result.set_connects( combine_connects( a, b ) );

        return result;
    }

    template<typename T>
    inline db_item_def<T>& operator+=( db_item_def<T> &a, const db_item_def<T> &b )
    {
        a.position = a.position + b.position;
        a.normal = a.normal + b.normal;
        a.uvw = a.uvw + b.uvw;

        a.weights = combine_weights( a.weights, b.weights );
        a.connects = combine_connects( a, b );

        return a;
    }

    template<typename T>
    inline db_item_def<T> operator*( const db_item_def<T> &a, real amount )
    {
        db_item_def<T> result(a);

        result.position = result.position * amount;
        result.normal = result.normal * amount;
        result.uvw = result.uvw * amount;

        for( auto &weight_pair : result.weights )
        {
            weight_pair.second *= amount;
        }

        return result;
    }

    template<typename T, typename W>
    inline typename VERTDB_ITERATOR_TRAITS<T>::value_type combine_defs( const T &begin, const T &end, const W &weight_begin, const W &weight_end, bool normalize=true )
    {
        VERTDB_ITERATOR_TRAITS<T>::value_type result;
        size_t weight_count = VERTDB_ITERATOR_DISTANCE( weight_begin, weight_end );

        if( VERTDB_ITERATOR_DISTANCE( begin, end ) != weight_count )
            return result;

        if( weight_count == 0 )
            return result;

        vd::real factor = 1;
        if( normalize )
        {
            vd::real total = 0;
            for( auto it = weight_begin; it != weight_end; ++it )
            {
                total += *it;
            }

            if( total > 0 )
                factor = 1 / total;
        }

        auto weight_it = weight_begin;
        for( auto it = begin; it != end; ++it, ++weight_it )
        {
            result += ( *it ) * (( *weight_it ) * factor);
        }

        return result;
    }

    template<typename T>
    inline typename VERTDB_ITERATOR_TRAITS<T>::value_type combine_defs( const T &begin, const T &end, bool normalize = true )
    {
        size_t count = VERTDB_ITERATOR_DISTANCE( begin, end );
        VERTDB_BUCKET< vd::real > weights( count, 1 );
        return combine_defs( begin, end, weights.begin(), weights.end(), normalize );
    }
};
