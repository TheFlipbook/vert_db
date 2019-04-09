#include "fixtures.h"

#include <math.h>
#include <sstream>

PointData add_random_ring( SimpleTestDB &db, size_t count )
{
    PointData points;
    SimpleRandom r;

    for( size_t i = 0; i < count; ++i )
    {
        vd::vec3 point{ r(), r(), r() };
        points.emplace_back( point );

        auto def = db.make_def();
        def.set_id( i );
        def.set_position( point );

        vd::vert_connects connects;
        connects.emplace_back( ( ( count + i ) - 1 ) % count );
        connects.emplace_back( ( ( count + i ) + 1 ) % count );
        def.set_connects( connects );

        db.insert( def );
    }

    return points;
}

PointData add_random_points( vd::point_cloud<size_t> &cloud, size_t count )
{
    PointData points;
    SimpleRandom r;

    for( size_t i = 0; i < count; ++i )
    {
        vd::vec3 point{ r(), r(), r() };
        points.emplace_back( point );
        cloud.insert( point, i );
    }

    return points;
}


size_t calc_sphere_key( size_t x, size_t y, size_t lat_count, size_t lon_count )
{
    return ( (y % lon_count) * lat_count ) + (x % lat_count);
}

size_t offset_sphere_key( size_t x, size_t y, size_t lat_count, size_t lon_count, int x_offset, int y_offset )
{
    // Can't offset off of the north pole
    if( ( y == 0 ) && ( y_offset < 0 ) )
        return -1;

    // Can't offset off of the south pole
    if( ( y == (lon_count - 1) ) && y_offset > 0 )
        return -1;

    size_t y_coord_unwrapped = ( y + lon_count ) + y_offset;
    size_t x_coord_unwrapped = ( x + lat_count ) + x_offset;

    size_t y_coord = y_coord_unwrapped % lon_count;
    size_t x_coord = x_coord_unwrapped % lat_count;

    return ( y_coord * lat_count ) + x_coord;
}


PointData add_sphere( SimpleTestDB &db, vd::real radius, size_t lat_count, size_t lon_count, vd::item_flags flags)
{
    PointData points;

    if( ( lat_count == 0 ) || ( lon_count <= 1 ) )
        return points;

    SimpleRandom r;

    vd::real theta_step = VERTDB_PI / (lon_count-1);
    vd::real phi_step = VERTDB_PI / lat_count;

    for( size_t y = 0; y < lon_count; ++y )
    {
        std::stringstream str;
        str << "joint_" << y;
        vd::bone_weight::first_type bone_name( str.str().c_str() );

        vd::real theta = theta_step * y;

        for( size_t x = 0; x < lat_count; ++x )
        {
            vd::real phi = phi_step * x;
            vd::real pos_x = sin( theta ) * cos( phi );
            vd::real pos_y = sin( theta ) * sin( phi );
            vd::real pos_z = cos( theta );

            vd::vec3 point{ pos_x * radius, pos_y * radius, pos_z * radius };
            points.emplace_back( point );

            vd::vec3 color{ abs( pos_x ), abs( pos_y ), abs( pos_z ) };

            size_t key = calc_sphere_key(x, y, lat_count, lon_count);
            size_t key_n = offset_sphere_key( x, y, lat_count, lon_count,  0, -1 );
            size_t key_s = offset_sphere_key( x, y, lat_count, lon_count,  0,  1 );
            size_t key_w = offset_sphere_key( x, y, lat_count, lon_count, -1,  0 );
            size_t key_e = offset_sphere_key( x, y, lat_count, lon_count,  1,  0 );

            vd::vert_connects connects{};
            if( key_n != -1 )
                connects.emplace_back( key_n );
            if( key_s != -1 )
                connects.emplace_back( key_s );
            if( key_w != -1 )
                connects.emplace_back( key_w );
            if( key_e != -1 )
                connects.emplace_back( key_e );

            vd::bone_weights weights;
            weights.emplace_back( bone_name, 1.0f );

            auto def = db.make_def();
            
            if( vd::flag_is_set(flags, vd::k_item_id) )
                def.set_id( key );

            if( vd::flag_is_set( flags, vd::k_item_position ) )
                def.set_position( point );

            if( vd::flag_is_set( flags, vd::k_item_color ) )
                def.set_color( color );

            if( vd::flag_is_set( flags, vd::k_item_weights ) )
                def.set_weights( weights );

            if( vd::flag_is_set( flags, vd::k_item_connects ) )
                def.set_connects( connects );

            db.insert( def );
        }

    }

    return points;
}
