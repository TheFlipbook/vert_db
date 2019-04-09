#pragma once

#include "vert_db/vert_db.h"

#include <random>

template<typename T>
struct RandomReal
{
    RandomReal( T low = 0.0f, T high = 10.0f )
        : m_generator()
        , m_distribution( low, high )
    {
    }

    T operator()()
    {
        return m_distribution( m_generator );
    }

    std::default_random_engine m_generator;
    std::uniform_real_distribution<vd::real> m_distribution;
};

typedef RandomReal<vd::real> SimpleRandom;
typedef vd::vert_db<size_t> SimpleTestDB;
typedef std::vector<vd::vec3> PointData;

PointData add_random_ring( SimpleTestDB &db, size_t count );
PointData add_random_points( vd::point_cloud<size_t> &cloud, size_t count );

size_t calc_sphere_key( size_t x, size_t y, size_t lat_count, size_t lon_count );
size_t offset_sphere_key( size_t x, size_t y, size_t lat_count, size_t lon_count, int x_offset, int y_offset );
PointData add_sphere( SimpleTestDB &db, vd::real radius, size_t lat_count = 20, size_t lon_count = 20, vd::item_flags flags=vd::k_item_all );

template<typename T>
void shuffle_ids( T &db )
{
    VERTDB_BUCKET<T::key_type> keys( db.begin(), db.end() );
    VERTDB_BUCKET<T::key_type> ids;
    for( const auto &key : keys )
    {
        ids.emplace_back( db.id( key ) );
    }

    std::shuffle( ids.begin(), ids.end(), std::default_random_engine() );
    
    for( size_t i = 0; i < ids.size(); ++i )
    {
        auto def = db.make_def();
        def.set_id( ids[i] );

        db.update( keys[i], def );
    }
}

template<typename T>
void flip_normals( T &db )
{
    for( const auto &key : db )
    {
        auto normal = db.normal( key );
        normal = normal * -1;

        auto def = db.make_def();
        if( db.gather( key, def ) )
        {
            def.normal = normal;
            db.update( key, def );
        }
    }
}

