#include "catch2/catch.hpp"

#include "fixtures.h"

TEST_CASE( "vert_db color queries", "[vert_db]" )
{
    const size_t sphere_dim = 20;
    const vd::real sphere_radius = 10;
    const vd::real color_tolerance = .05f;
    vd::item_flags dest_flags = vd::k_item_all;

    SimpleTestDB db;
    add_sphere( db, sphere_radius, sphere_dim, sphere_dim, dest_flags );

    // Get a pole vertex
    vd::vec3 top_pole{ 0, 0, sphere_radius };
    auto pole_verts = db.find_position( top_pole );
    REQUIRE( !pole_verts.empty() );

    // Should be able to find at least one vert with our exact color (at least the query)
    auto color = db.color(pole_verts.front());
    auto color_matched = db.find_color(color);
    REQUIRE( !color_matched.empty() );

    // Shouldn't have found every vert
    REQUIRE( color_matched.size() != db.size() );

    size_t mismatched_finds = 0;
    for( auto key : color_matched )
    {
        if( db.distance_to_color( color, key ) > color_tolerance )
        {
            ++mismatched_finds;
        }
    }

    REQUIRE( mismatched_finds == 0 );

    vd::vec3 biased_sample{ sphere_radius, sphere_radius/2, sphere_radius/2 };
    auto sampled = db.sample_color( biased_sample, sphere_radius );

    // Averaged offset query should probably not match the Z pole
    REQUIRE( !(sampled == color) );
}
