#include "catch2/catch.hpp"

#include "fixtures.h"

#include "vert_db/vert_db_transfer_utils.h"

TEST_CASE( "skin_wrap transfers positions match", "[skin_wrap]" )
{
    const size_t sphere_dim = 20;
    const vd::real sphere_radius = 10;
    vd::item_flags dest_flags = vd::flag_without( vd::k_item_all, vd::k_item_weights );

    vd::item_flags set_flags = vd::k_item_weights;

    vd::transfer_db<size_t> skinner;
    skinner.add_resolver< vd::transfer_resolver_matched<size_t> >(set_flags);
    add_sphere( skinner.vert_db(), sphere_radius, sphere_dim, sphere_dim );

    vd::vert_db<size_t> results;
    add_sphere( results, sphere_radius, sphere_dim, sphere_dim, dest_flags );

    // Skinweights must be empty before transfer
    size_t probe = skinner.vert_db().size() / 4;
    REQUIRE( results.weights( probe ).empty() );

    vd::vert_db<size_t> scrambled;
    add_sphere( scrambled, sphere_radius, sphere_dim, sphere_dim, dest_flags );
    shuffle_ids( scrambled );

    // Scrambled IDs must not match default IDs
    REQUIRE( scrambled != results );
    REQUIRE( !scrambled.channel_equal(results, vd::k_item_id) );

    skinner.apply( results );

    // Skinweights should be set after apply
    REQUIRE( !(results.weights( probe ).empty()) );
    REQUIRE( results.weights( probe ) == skinner.vert_db().weights( probe ) );

    skinner.add_resolver<vd::transfer_resolver_position<size_t> >( set_flags );
    skinner.apply( scrambled );
    
    // After transfer scrambled and exact should be the same.
    REQUIRE( scrambled.channel_equal( results, vd::k_item_weights | vd::k_item_position ) );
}

TEST_CASE( "skin_wrap transfers and floods missing data", "[skin_wrap]" )
{
    const size_t sphere_dim = 15;   // Need odd number of items for this test, because equator is needed
    const vd::real sphere_radius = 10;
    const size_t probe = calc_sphere_key( 0, sphere_dim / 2, sphere_dim, sphere_dim );
    const size_t probe_up = offset_sphere_key( 0, sphere_dim / 2, sphere_dim, sphere_dim, 0, -1 );
    const size_t probe_down = offset_sphere_key( 0, sphere_dim / 2, sphere_dim, sphere_dim, 0,  1 );

    std::string joint_a = "joint_0";
    std::string joint_b = "joint_1";
    vd::vec3 top_pole{ 0, 0, sphere_radius };
    vd::vec3 bot_pole{ 0, 0, -sphere_radius };
    vd::item_flags dest_flags = vd::flag_without( vd::k_item_all, vd::k_item_weights );
    vd::item_flags set_flags = vd::k_item_weights;

    // Set up unskinned sphere
    SimpleTestDB dest_data;
    add_sphere( dest_data, sphere_radius, sphere_dim, sphere_dim, dest_flags );

    // Transfer DB has two points, weighted at the poles
    vd::transfer_db<size_t> skinner;
    skinner.add_resolver< vd::transfer_resolver_position<size_t> >( set_flags );
    skinner.add_resolver< vd::transfer_flood_fill<size_t> >( set_flags );

    auto top = skinner.vert_db().make_def();
    top.set_position( top_pole );
    top.set_weights( vd::bone_weights{ vd::bone_weight{joint_a, 1.0f} } );
    skinner.vert_db().insert( top );

    auto bot = skinner.vert_db().make_def();
    bot.set_position( bot_pole );
    bot.set_weights( vd::bone_weights{ vd::bone_weight{ joint_b, 1.0f } } );
    skinner.vert_db().insert( bot );

    // Testing data must be different for this test to be meaningful
    REQUIRE( top.weights != bot.weights );

    // Transfer points to the poles, then flood fill
    skinner.apply( dest_data );

    // Points near poles should be solid.
    bool top_pole_correct = true;
    auto found_top = dest_data.find_position( top_pole );
    REQUIRE( !found_top.empty() );

    for( const auto &point_id : found_top )
    {
        if( top.weights != dest_data.weights( point_id ) )
        {
            top_pole_correct = false;
            break;
        }
    }

    bool bot_pole_correct = true;
    auto found_bot = dest_data.find_position( bot_pole );
    REQUIRE( !found_bot.empty() );

    for( const auto &point_id : found_bot )
    {
        if( bot.weights != dest_data.weights( point_id ) )
        {
            bot_pole_correct = false;
            break;
        }
    }

    REQUIRE( top_pole_correct );
    REQUIRE( bot_pole_correct );

    // Points at equator should be mixed.
    auto equator_weights = dest_data.weights( probe );
    REQUIRE( equator_weights.size() > 1 );

    // Points near equator should be one or the other though
    auto up_weights = dest_data.weights( probe_up );
    REQUIRE( up_weights == top.weights );

    auto down_weights = dest_data.weights( probe_down );
    REQUIRE( down_weights == bot.weights );
}
