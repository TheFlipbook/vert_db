#pragma once

#include "vert_db_config.h"

#include "vert_db_types.h"
#include "vert_db_item.h"
#include "vert_db_utils.h"
#include "vert_db_cloud.h"

#define VERTDB_MEMBER_CHECK(field, compare) \
{                                           \
    if( field != compare##.##field )        \
        return false;                       \
}

namespace vd
{
    template<typename T, typename S = real>
    class vert_db
    {
    public:
        typedef vert_db<T, S> self_type;
        typedef T value_type;
        typedef vec3 point_type;
        typedef vec3i point_key_type;
        typedef vert_id key_type;
        typedef S scalar;
        typedef bone_weights bone_weights_type;
        typedef vert_connects vert_connects_type;

        typedef VERTDB_DATA_STORAGE<value_type> value_collection;

        typedef VERTDB_MAP<vert_id, key_type> vert_directory;

        typedef VERTDB_SET<key_type> vert_manifest;
        typedef VERTDB_MAP<key_type, key_type> id_storage;
        typedef VERTDB_MAP<key_type, point_type> point_storage;
        typedef VERTDB_MAP<key_type, bone_weights_type> weights_storage;
        typedef VERTDB_MAP<key_type, vert_connects_type> connects_storage;

        typedef db_item_def<value_type> def_type;
        typedef point_cloud<key_type, point_type, point_key_type, scalar> cloud_type;

        typedef VERTDB_BUCKET<key_type> key_collection;
        typedef key_collection results_type;
        typedef VERTDB_SET<key_type> key_set;
        typedef VERTDB_BUCKET<scalar> scalar_collection;

        typedef typename const vert_manifest::iterator const_iterator;

        typedef VERTDB_NUMERIC_LIMITS<scalar> limits_type;

        static inline constexpr scalar epsilon()
        {
            return limits_type::epsilon() * VERTDB_EPSILON_SCALE;
        }

        static inline def_type make_def()
        {
            def_type result;
            return result;
        }

        vert_db()
            : m_data()
            , m_manifest()
            , m_directory()
            , m_ids()
            , m_positions()
            , m_normals()
            , m_uvws()
            , m_weights()
            , m_connects()
            , m_pos_cloud()
            , m_uvw_cloud()
            , m_mutex_edit()
        {
        }

        inline bool gather(const key_type &key, def_type &result) const
        {
            auto found = m_manifest.find( key );
            if( found == m_manifest.end() )
                return false;

            result.gather_id( key, m_ids );
            result.gather_position( key, m_positions );
            result.gather_normal( key, m_normals );
            result.gather_uvw( key, m_uvws );
            result.gather_weights( key, m_weights );
            result.gather_connects( key, m_connects );

            return true;
        }

        size_t size() const
        {
            return m_data.size();
        }

        const_iterator begin() const
        {
            return m_manifest.begin();
        }

        const_iterator end() const
        {
            return m_manifest.end();
        }

        key_type insert( const def_type &def )
        {
            key_type key = m_data.size();
            m_data.emplace_back( def.user_data );

            apply_def( key, def );
            return key;
        }

        key_type insert_atomic( const def_type &def )
        {
            lock_type lock( m_mutex_edit );
            return insert( def );
        }

        self_type operator+( const self_type &other )
        {
            self_type ret;

            for( const auto &key : *this )
            {
                auto def = make_def();
                if( gather( key, def ) )
                {
                    insert( def );
                }
            }

            for( const auto &key : other )
            {
                auto def = make_def();
                if( other.gather( key, def ) )
                {
                    insert( def );
                }
            }

            return ret;
        }

        self_type& operator+=( const self_type &other )
        {
            for( const auto &key : other )
            {
                auto def = make_def();
                if( other.gather( key, def ) )
                {
                    insert( def );
                }
            }

            return &this;
        }

        void update( const key_type &key, const def_type &def )
        {
            apply_def( key, def );
        }

        void update_atomic( const key_type &key, const def_type &def )
        {
            lock_type lock( m_mutex_edit );
            update( key, def );
        }

        key_type find_id( const vert_id &id ) const
        {
            auto found = m_directory.find( id );
            if( found == m_directory.end() )
                return c_invalid_vert_id;

            return found->second;
        }

        results_type find_position( const point_type &location, scalar radius=epsilon() ) const
        {
            return m_pos_cloud.find( location, radius );
        }

        results_type find_uvw( const point_type &location, scalar radius=epsilon() ) const
        {
            return m_uvw_cloud.find( location, radius );
        }

        inline results_type find_connects( const key_type &key, size_t depth = 1, bool inclusive = false ) const
        {
            key_collection frontier;
            frontier.emplace_back( key );
            return find_connects( frontier, depth, inclusive );
        }

        results_type find_connects( key_collection &frontier, size_t depth=1, bool inclusive=false ) const
        {
            size_t cursor = 0;
            size_t current_depth = 0;
            size_t sentinal = frontier.size();
            size_t first = ( inclusive ) ? 0 : sentinal;

            key_set seen( frontier.begin(), frontier.end() );

            while( cursor < frontier.size() )
            {
                const key_type &current_key = frontier[cursor];
                auto found_connects = m_connects.find( current_key );
                if( found_connects != m_connects.end() )
                {
                    const auto &connects = found_connects->second;
                    for( const auto &connect : connects )
                    {
                        key_type found_key = find_id( connect );
                        if( found_key != c_invalid_vert_id )
                        {
                            auto found_seen = seen.find( found_key );
                            if( found_seen == seen.end() )
                            {
                                frontier.emplace_back( found_key );
                                seen.emplace( found_key );
                            }
                        }
                    }
                }

                ++cursor;

                if( cursor >= sentinal )
                {
                    ++current_depth;
                    if( current_depth >= depth )
                        break;

                    sentinal = frontier.size();
                }
            }

            results_type results( frontier.begin() + first, frontier.end() );
            return results;
        }

        static bool weight_sort( const bone_weight &a, const bone_weight &b )
        {
            return a.second > b.second;
        }

        bone_weights find_weights(const point_type &location, real radius, size_t total=0, real clip=.1f, bool normalize=true) const
        {
            bone_weights results;

            results_type verts = find_position( location, radius );
            if( verts.empty() )
                return results;

            size_t count = verts.size();
            scalar_collection distances = distances_to( location, verts );

            // We'll use a radius factor instead of the standard deviation
            //  Because the user probably intends a filter from their query point
            //  Not a weighting around what was in the radius query, in case those results were biased.
            //scalar sigma = deviation<scalar_collection>( distances.begin(), distances.end() );
            scalar sigma = radius / 2;

            for( size_t i = 0; i < count; ++i )
            {
                scalar weighting = gaussian_weight( distances[i], sigma );
                accumulate_weight( results, verts[i], weighting );
            }

            if( (total > 0) && (results.size() > total) )
            {
                VERTDB_BUCKET_SORTER( results.begin(), results.end(), weight_sort );
                results.resize( total );
            }

            if( normalize )
            {
                normalize_weights( results );
            }

            clip_weights( results, clip, normalize );

            return results;
        }

        inline vert_id id( key_type key ) const
        {
            return basic_query( key, m_ids );
        }

        inline point_type position(key_type key) const
        {
            return basic_query( key, m_positions );
        }

        inline point_type normal( key_type key ) const
        {
            return basic_query( key, m_normals );
        }

        inline point_type uvw( key_type key ) const
        {
            return basic_query( key, m_uvws );
        }

        inline bone_weights weights( key_type key ) const
        {
            return basic_query( key, m_weights );
        }

        inline vert_connects connects( key_type key ) const
        {
            return basic_query( key, m_connects );
        }

        inline value_type& user_data( key_type key ) const
        {
            return m_data[key];
        }

        inline scalar distance_to( const point_type &point, key_type key ) const
        {
            point_type between = position( key ) - point;
            scalar dist_sq = dot( between, between );

            return sqrt( dist_sq );
        }

        inline scalar distance_to_uvw( const point_type &point, key_type key ) const
        {
            point_type between = uvw( key ) - point;
            scalar dist_sq = dot( between, between );

            return sqrt( dist_sq );
        }

        bool channel_equal( const self_type &other, item_flags flags ) const
        {
            if( flag_is_set(flags, k_item_id ) )
                VERTDB_MEMBER_CHECK( m_ids, other );

            if( flag_is_set( flags, k_item_position ) )
                VERTDB_MEMBER_CHECK( m_positions, other );

            if( flag_is_set( flags, k_item_normal ) )
                VERTDB_MEMBER_CHECK( m_normals, other );

            if( flag_is_set( flags, k_item_uvw ) )
                VERTDB_MEMBER_CHECK( m_uvws, other );

            if( flag_is_set( flags, k_item_weights ) )
                VERTDB_MEMBER_CHECK( m_weights, other );

            if( flag_is_set( flags, k_item_connects ) )
                VERTDB_MEMBER_CHECK( m_connects, other );

            if( flag_is_set( flags, k_item_user_data ) )
                VERTDB_MEMBER_CHECK( m_data, other );

            return true;
        }

        bool operator==( const self_type &other ) const
        {
            VERTDB_MEMBER_CHECK( size(), other );
            VERTDB_MEMBER_CHECK( m_manifest, other );
            VERTDB_MEMBER_CHECK( m_directory, other );

            return channel_equal( other, k_item_all );
        }

        bool operator!=( const self_type &other ) const
        {
            return !( *this == other );
        }

    protected:

        inline scalar_collection distances_to( const point_type &location, key_collection &verts ) const
        {
            scalar_collection distances;
            distances.reserve( verts.size() );
            for( const auto &key : verts )
            {
                real dist = distance_to( location, key );
                distances.emplace_back( dist );
            }

            return distances;
        }

        inline bool accumulate_weight( bone_weights &results, const key_type &key, scalar modifier ) const
        {
            bool added_weights = false;

            const bone_weights &weight_data = weights( key );
            for( const auto &weight : weight_data )
            {
                weight_finder finder( weight.first );
                auto &found = find_if( results.begin(), results.end(), finder );
                if( found == results.end() )
                {
                    results.emplace_back( weight );
                    added_weights = true;
                }
                else
                {
                    found->second += weight.second * modifier;
                    added_weights = false;
                }
            }

            return added_weights;
        }

        inline bool clip_weights( bone_weights &results, scalar clip, bool normalize ) const
        {
            bool did_clip = false;

            if( clip > 0 )
            {
                for( size_t i = results.size(); i > 0; --i )
                {
                    size_t index = i - 1;
                    if( results[index].second < clip )
                    {
                        results.erase( results.begin() + index );
                        did_clip = true;
                    }
                }

                if( normalize && did_clip )
                {
                    normalize_weights( results );
                }
            }

            return did_clip;
        }

        static inline bool normalize_weights( bone_weights &weights )
        {
            scalar sum = 0;
            for( const auto &weight : weights )
            {
                sum += weight.second;
            }

            if( sum > 0 )
            {
                scalar factor = 1 / sum;
                for( auto &weight : weights )
                {
                    weight.second *= factor;
                }

                return true;
            }

            return false;
        }

        template<typename C>
        inline typename C::mapped_type basic_query( const key_type &key, const C &storage ) const
        {
            auto found = storage.find( key );
            if( found != storage.end() )
                return found->second;

            C::mapped_type result{};
            return result;
        }

        void apply_def(key_type key, const def_type &def)
        {
            m_manifest.emplace( key );

            // Raw Data
            def.apply_id( key, m_ids );
            def.apply_position( key, m_positions );
            def.apply_normal( key,  m_normals );
            def.apply_uvw( key, m_uvws );
            def.apply_weights( key, m_weights );
            def.apply_connects( key, m_connects );

            // Accelleration structures
            if( def.has_id() )
                m_directory[def.id] = key;

            if( def.has_position() )
                m_pos_cloud.insert( def.position, key );

            if( def.has_uvw() )
                m_uvw_cloud.insert( def.uvw, key );
        }

        // Authoritative representation
        value_collection m_data;
        vert_manifest m_manifest;

        // Remap from user keys to internal keys
        vert_directory m_directory;

        // Internal data storage (keys may be sparse)
        id_storage m_ids;
        point_storage m_positions;
        point_storage m_normals;
        point_storage m_uvws;
        weights_storage m_weights;
        connects_storage m_connects;

        // Accelleration Structures
        cloud_type m_pos_cloud;
        cloud_type m_uvw_cloud;

        // Parellelization
        mutex_type m_mutex_edit;
    };
};
