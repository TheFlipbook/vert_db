#include "catch2/catch.hpp"

#include "fixtures.h"

TEST_CASE( "vert_db skin weight queries", "[vert_db]" )
{
    const vd::real sphere_radius = 10;
    const size_t sphere_dim = 20;
    const size_t sample_x = 0;
    const size_t sample_y = 5;

    const vd::real arc = ( sphere_radius * 2 * VERTDB_PI ) / sphere_dim;

    // Should be able to add to the point cloud
    SimpleTestDB db;
    std::vector<vd::vec3> points = add_sphere(db, sphere_radius, sphere_dim, sphere_dim );

    // Can't logically perform this test without many verts
    REQUIRE( db.size() > 4 );

    size_t probe_a = calc_sphere_key( sample_x, sample_y, sphere_dim, sphere_dim ); // Pick arbitrary point
    size_t probe_b = offset_sphere_key( sample_x, sample_y,  sphere_dim, sphere_dim, 0, 1);

    // Slightly biased probe location
    vd::vec3 probe = (( db.position( probe_a ) - db.position( probe_b ) ) * .25f) + db.position( probe_b );
    vd::bone_weights weights = db.find_weights( probe, arc, 4 );

    vd::real sum = 0;
    for( const auto &weight : weights )
    {
        sum += weight.second;
    }

    // We should have found normalized weights with this small location
    REQUIRE( sum >= ( 1 - db.epsilon() ) );
    REQUIRE( sum <= ( 1 + db.epsilon() ) );

    // Can miss Weight probes;
    vd::vec3 miss_probe{ 20000, 20000, 20000 };
    vd::bone_weights miss = db.find_weights( miss_probe, .25f, 4 );
    REQUIRE( miss.size() == 0 );
}
