#pragma once

#include "vert_db_types.h"

namespace vd
{
    inline size_t hash_combine( size_t seed )
    {
        return seed;
    }

    template <typename T, typename ...Rest>
    inline size_t hash_combine( size_t seed, const T& v, Rest ...rest)
    {
        std::hash<T> hasher;
        size_t next = seed ^ (hasher( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 ));
        return hash_combine( next, rest... );
    }

    inline bool operator==( const vec3i &a, const vec3i &b )
    {
        return ( a.x == b.x ) && ( a.y == b.y ) && ( a.z == b.z );
    }

    template<typename T>
    inline bool near_equal( const T &a, const T &b, const T &epsilon )
    {
        return( abs( a - b ) < epsilon );
    }

    inline bool operator==( const vec3_internal &a, const vec3_internal &b )
    {
        const vd::real eps = VERTDB_NUMERIC_LIMITS<vd::real>::epsilon() * VERTDB_EPSILON_SCALE;
        
        if( !near_equal( a.x, b.x, eps ) )
            return false;

        if( !near_equal( a.y, b.y, eps ) )
            return false;

        if( !near_equal( a.z, b.z, eps ) )
            return false;

        return true;;
    }

    inline vec3_internal operator+( const vec3_internal &a, const vec3_internal &b )
    {
        vec3_internal result{ a.x + b.x, a.y + b.y, a.z + b.z };
        return result;
    }

    inline vec3_internal operator-( const vec3_internal &a, const vec3_internal &b )
    {
        vec3_internal result{ a.x - b.x, a.y - b.y, a.z - b.z };
        return result;
    }

    inline vec3_internal operator*( const vec3_internal &a, vd::real scale )
    {
        vec3_internal result{ a.x * scale, a.y * scale, a.z * scale };
        return result;
    }

    inline real dot( const vec3 &a, const vec3 &b )
    {
        return ( a.x * b.x ) + ( a.y * b.y ) + ( a.z * b.z );
    }

    template<typename T>
    inline vec3i floor_vec3i( const T& v )
    {
        return { static_cast<int_t>(std::floor( v.x )),
                 static_cast<int_t>(std::floor( v.y )),
                 static_cast<int_t>(std::floor( v.z )) };
    }

    template<typename K, typename T>
    struct to_key
    {
        K operator()( const T& v )
        {
            return K( v );
        }
    };

    template<typename T>
    struct to_key<vec3i, T>
    {
        vec3i operator()( const T& v )
        {
            return floor_vec3i( v );
        }
    };

    inline vec3& splat( vec3& vec, real value )
    {
        vec.x = value;
        vec.y = value;
        vec.z = value;
        return vec;
    }

    inline VERTDB_BUCKET<vec3i> flood( const vec3i &low, const vec3i &high )
    {
        VERTDB_BUCKET<vec3i> result;

        int_t low_x = ( low.x <= high.x ) ? low.x : high.x;
        int_t low_y = ( low.y <= high.y ) ? low.y : high.y;
        int_t low_z = ( low.z <= high.z ) ? low.z : high.z;

        int_t high_x = ( low.x > high.x ) ? low.x : high.x;
        int_t high_y = ( low.y > high.y ) ? low.y : high.y;
        int_t high_z = ( low.z > high.z ) ? low.z : high.z;

        for( int_t x = low_x; x <= high_x; ++x )
        {
            for( int_t y = low_y; y <= high_y; ++y )
            {
                for( int_t z = low_z; z <= high_z; ++z )
                {
                    result.emplace_back( vec3i{ x, y, z } );
                }
            }
        }

        return result;
    }

    template<typename T>
    typename T::value_type deviation( const typename T::iterator &begin, const typename T::iterator &end )
    {
        real sum{};

        size_t total = 0;
        for( auto it = begin; it != end; ++it )
        {
            sum += *it;
            ++total;
        }

        auto mean = sum / total;

        real deviation_sum{};
        for( auto it = begin; it != end; ++it )
        {
            auto val = ( *it ) - mean;
            deviation_sum += val * val;
        }

        return sqrt( deviation_sum / total );
    }

    const real c_sqrt_inv_2pi = sqrt(1 / ( 2 * VERTDB_PI ));

    template<typename T>
    T gaussian_weight( T value, T dev )
    {
        T a = value / dev;
        return ( c_sqrt_inv_2pi / dev) * std::exp( -.5 * a * a );
    }
}

namespace std
{
    template<>
    struct hash<vd::vec3i>
    {
        size_t operator()( const vd::vec3i &v ) const
        {
            return vd::hash_combine(0, v.x, v.y, v.z);
        }
    };
}
