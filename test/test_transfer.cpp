#include "catch2/catch.hpp"

#include "fixtures.h"

#include "vert_db/vert_db_transfer_utils.h"

TEST_CASE( "transfer physical works correctly", "[vert_db]" )
{
    const size_t sphere_dim = 20;
    const vd::real sphere_radius = 10;
    const vd::real kernel_radius = 9.5;
    const size_t sample_x = 0;
    const size_t sample_y = 5;
    const vd::real sample_radius = .05;

    vd::item_flags dest_flags = vd::flag_without( vd::k_item_all, vd::k_item_weights );
    vd::item_flags set_flags = vd::k_item_weights;

    vd::transfer_db<size_t> skinner;
    skinner.add_resolver< vd::transfer_resolver_physical<size_t> >(set_flags);

    // Add a sphere with exact positions but flipped normals
    add_sphere( skinner.vert_db(), sphere_radius, sphere_dim, sphere_dim );
    flip_normals( skinner.vert_db() );

    // Add a slightly smaller sphere with correct normals
    add_sphere( skinner.vert_db(), kernel_radius, sphere_dim, sphere_dim );

    // Create output DB
    SimpleTestDB db;
    std::vector<vd::vec3> points = add_sphere(db, sphere_radius, sphere_dim, sphere_dim );

    size_t probe_index = calc_sphere_key( sample_x, sample_y, sphere_dim, sphere_dim ); // Pick arbitrary point
    vd::vec3 probe = db.position( probe_index );
    vd::vec3 probe_kernel = probe * (kernel_radius / sphere_radius);

    skinner.apply( db ); // Do full transfer

    // TODO: need negative test here too.
    REQUIRE( db.find_weights(probe, sample_radius) == skinner.vert_db().find_weights(probe_kernel, sample_radius));
}