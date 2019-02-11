#pragma once

#include "vert_db_config.h"
#include "vert_db_types.h"
#include "vert_db_item.h"
#include "vert_db_utils.h"
#include "vert_db_thread.h"

namespace vd
{
    template<typename T, typename V = vec3, typename K = vec3i, typename S=real>
    class point_cloud
    {
    public:
        typedef point_cloud<T, V, K> self_type;
        typedef T mapped_type;
        typedef V point_type;
        typedef K key_type;
        typedef S scalar;

        typedef VERTDB_BUCKET<mapped_type> results_type;
        typedef VERTDB_BUCKET<key_type> key_collection;
        typedef VERTDB_PAIR<point_type, mapped_type> bucket_value_type;
        typedef VERTDB_BUCKET<bucket_value_type> bucket_type;

        typedef VERTDB_MAP<key_type, bucket_type> bucket_map;
        typedef typename bucket_map::value_type map_pair;

        typedef VERTDB_NUMERIC_LIMITS<scalar> limits_type;

        static inline constexpr scalar epsilon()
        {
            return limits_type::epsilon() * VERTDB_EPSILON_SCALE;
        }

        point_cloud(scalar bucket_dim=1)
            : m_bucket_scale( width_to_scale(bucket_dim) )
            , m_data()
        {
        }

        virtual ~point_cloud()
        {
        }

        size_t size()
        {
            return m_data.size();
        }

        inline key_type key( const point_type &location ) const
        {
            to_key<key_type, point_type> keyer;
            return keyer( location * m_bucket_scale );
        }

        inline key_collection grid_keys( const point_type &location, const point_type &half_size ) const
        {
            point_type low( location - half_size );
            point_type high( location + half_size );

            key_type low_key = key( low );
            key_type high_key = key( high );

            return flood( low_key, high_key );
        }

        inline key_collection grid_keys( const point_type &location, scalar radius ) const
        {
            point_type half_size;
            splat( half_size, radius );

            return grid_keys( location, half_size );
        }

        void insert( const point_type &location, const mapped_type &item )
        {
            key_type index = key( location );

            auto found = m_data.find( index );
            if( found == m_data.end() )
            {
                auto result = m_data.emplace( map_pair( index, {} ) );
                found = result.first;
            }

            found->second.emplace_back( location, item );
        }

        void rebucket( scalar bucket_width )
        {
            // Store data
            bucket_map temp( std::move( m_data ) );

            // Reset structure
            m_data.clear();
            m_bucket_scale = width_to_scale( bucket_width );

            // Reinsert all the data
            for( auto &bucket : temp )
            {
                for( auto &pair : bucket.second )
                {
                    key_type index = key( pair.first );
                    auto found = m_data.find( index );
                    if( found == m_data.end() )
                    {
                        auto result = m_data.emplace( map_pair( index, {} ) );
                        found = result.first;
                    }

                    found->second.emplace_back( std::move(pair) );
                }
            }
        }

        results_type find_bucket( const point_type& location, scalar radius, const key_type &key ) const
        {
            results_type results;

            scalar rad_sq = radius * radius;

            auto found = m_data.find( key );
            if( found != m_data.end() )
            {
                const auto& bucket = found->second;
                for( const auto& item : bucket )
                {
                    V between = location - item.first;
                    scalar dist_sq = dot( between, between );
                    if( dist_sq <= rad_sq )
                    {
                        results.emplace_back( item.second );
                    }
                }
            }

            return results;
        }

        results_type find( const point_type &location, scalar radius=epsilon() ) const
        {
            results_type results;
            key_collection keys = grid_keys( location, radius );

            bucket_processor_func bucket_runner{ *this, location, radius };
            bucket_processor processor( bucket_runner, keys.begin(), keys.end(), results );
            processor.join();

            return results;
        }

    protected:

        scalar width_to_scale( scalar bucket_dim ) const
        {
            return ( bucket_dim == 0 ) ? epsilon() : 1 / bucket_dim;
        }

        struct bucket_processor_func
        {
            void operator()( const key_type &key, results_type &collector ) const
            {
                auto values = m_cloud.find_bucket( m_location, m_radius, key );
                collector.insert( collector.end(), values.begin(), values.end() );
            }

            const self_type &m_cloud;
            point_type m_location;
            scalar m_radius;
        };

        typedef threaded_processor<bucket_processor_func, typename key_collection::iterator, results_type> bucket_processor;

        scalar m_bucket_scale;
        bucket_map m_data;
    };
};
