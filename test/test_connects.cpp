#include "catch2/catch.hpp"

#include "fixtures.h"

TEST_CASE( "vert_db connectivity queries", "[vert_db]" )
{
    SimpleTestDB db;
    std::vector<vd::vec3> points = add_random_ring( db, 25 );

    // Points are connected in a ring, so 2 deep should be 4 points exactly
    auto exclusive_connects = db.find_connects( 0, 2, false );
    REQUIRE( exclusive_connects.size() == 4 );

    // Inclusive should match exclusive but be one larger
    auto inclusive_connects = db.find_connects( 0, 2, true );
    REQUIRE( inclusive_connects.size() == exclusive_connects.size() + 1 );

    // A slice of the connect searches excluding the inclusive portion should match
    vd::vert_connects slice_connects( inclusive_connects.begin() + 1, inclusive_connects.end() );
    REQUIRE( slice_connects == exclusive_connects );

    // Results from one deep search should match two progressive searches
    auto shallow_connects = db.find_connects( 0, 1, true );
    auto expanded_connects = db.find_connects( shallow_connects, 1, true );
    REQUIRE( expanded_connects == inclusive_connects );
}
