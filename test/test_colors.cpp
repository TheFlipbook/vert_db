#include "catch2/catch.hpp"

#include "fixtures.h"

#include "vert_db/vert_db_transfer_utils.h"

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
    REQUIRE( !vd::near_equal(sampled, color, color_tolerance) );
}

TEST_CASE( "vert_db color transfer", "[vert_db]" )
{
    const size_t src_sphere_dim = 20;
    const size_t dest_sphere_dim = 30;
    const vd::real sphere_radius = 10;
    const vd::real filter_radius = sphere_radius / 10;
    const vd::real invalid_radius = sphere_radius * 100;
    const vd::real color_tolerance = .05f;
    vd::item_flags src_flags = vd::k_item_all;
    vd::item_flags dest_flags = flag_without(vd::k_item_all, vd::k_item_color);

    // Set up blank sphere as target
    SimpleTestDB dest_data;
    add_sphere( dest_data, sphere_radius, dest_sphere_dim, dest_sphere_dim, dest_flags );

    // Transfer DB has one fully specified sphere
    vd::transfer_db<size_t> source;
    source.add_resolver< vd::transfer_resolver_matched<size_t> >( src_flags );
    source.add_resolver< vd::transfer_resolver_position<size_t> >( src_flags );
    source.add_resolver< vd::transfer_resolver_gaussian<size_t> >( src_flags, filter_radius );
    source.add_resolver< vd::transfer_flood_fill<size_t> >( src_flags );

    add_sphere( source.vert_db(), sphere_radius, src_sphere_dim, src_sphere_dim, src_flags );

    // Do the transfer
    bool transferred = source.apply( dest_data );
    REQUIRE( transferred );

    // Get a pole vertex
    vd::vec3 top_pole{ 0, 0, sphere_radius };

    auto source_pole_verts = source.vert_db().find_position( top_pole );
    REQUIRE( !source_pole_verts.empty() );

    auto dest_pole_verts = dest_data.find_position( top_pole );
    REQUIRE( !dest_pole_verts.empty() );

    auto source_color = source.vert_db().color( source_pole_verts.front() );
    auto dest_color = dest_data.color( dest_pole_verts.front() );

    // Pole vertices should match color very closely
    REQUIRE( dest_data.distance_to_color(source_color, dest_pole_verts.front()) < color_tolerance );

    vd::vec3 invalid_sample{ 0, 0, invalid_radius };
    vd::vec3 biased_sample{ sphere_radius, sphere_radius / 2, sphere_radius / 2 };

    vd::real norm_factor = sqrt( biased_sample.x*biased_sample.x + biased_sample.y*biased_sample.y + biased_sample.z*biased_sample.z );
    biased_sample = biased_sample * ( sphere_radius / norm_factor );

    auto source_sampled = source.vert_db().sample_color( biased_sample, filter_radius );
    auto dest_sampled = dest_data.sample_color( biased_sample, filter_radius );
    auto dest_invalid = dest_data.sample_color( invalid_sample, filter_radius );

    // Invalid sample should have null data compared to a real one
    REQUIRE( !vd::near_equal(dest_sampled, dest_invalid, color_tolerance ) );

    // Real samples on sphere surface should be very similar even across lossy point locations
    REQUIRE( vd::near_equal( dest_sampled, source_sampled, color_tolerance ) );

    // Averaged offset query should probably not match the Z pole
    REQUIRE( !vd::near_equal(dest_sampled, dest_color, color_tolerance) );
}
