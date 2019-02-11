#include "catch2/catch.hpp"

#include "fixtures.h"

TEST_CASE( "vert_db point queries", "[vert_db]" )
{
    SimpleTestDB db;
    std::vector<vd::vec3> points = add_random_ring( db, 100 );

    vd::real check_dist = .01f;
    vd::real check_dist_sq = check_dist * check_dist;

    size_t found_hits = 0;
    for( const auto &point : points )
    {
        auto keys = db.find_position( point, .1f );
        for( auto key : keys )
        {
            vd::vec3 pos = db.position( key );
            vd::vec3 between = point - pos;
            vd::real dist_sq = vd::dot( between, between );
            if( dist_sq <= check_dist_sq )
            {
                ++found_hits;
                break;
            }
        }
    }

    // Should have been able to find an exact hit for every inserted point
    REQUIRE( found_hits == points.size() );
}