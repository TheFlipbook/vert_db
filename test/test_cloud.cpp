#include "catch2/catch.hpp"

#include "fixtures.h"

TEST_CASE( "point_cloud basic functions", "[point_cloud]" )
{
    // Should be able to add to the point cloud
    vd::point_cloud<size_t> cloud;
    cloud.insert( vd::vec3{ 1.0f, 2.2f, 3.8f }, 0 );
    REQUIRE( cloud.size() == 1 );

    // vert_db test data should initialize to desired sizes.
    //  vert_db and the verification data should agree on this size.
    vd::vert_db<size_t> db;
    size_t count = 100;
    std::vector<vd::vec3> points = add_random_ring( db, count );
    REQUIRE( db.size() == count );
    REQUIRE( db.size() == points.size() );
}


TEST_CASE( "point_cloud resizing", "[point_cloud]" )
{
    vd::vert_db<size_t> db;
    size_t count = 100;

    vd::point_cloud<size_t> cloud;
    std::vector<vd::vec3> points = add_random_points( cloud, count );

    // Resize the cloud and make sure we have different keys
    size_t bucket_count_before = cloud.size();
    cloud.rebucket( .05 );
    size_t bucket_count_after = cloud.size();
    REQUIRE( bucket_count_before != bucket_count_after );

    // Validate that all the points are still there
    size_t found_hits = 0;
    for( const auto &point : points )
    {
        auto keys = cloud.find( point );
        if( !keys.empty() )
            ++found_hits;
    }

    REQUIRE( found_hits == points.size() );
}
