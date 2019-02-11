#pragma once

#include "vert_db_config.h"

namespace vd
{
    typedef VERTDB_SCALAR real;

    typedef VERTDB_INTEGER int_t;

    struct vec3_internal
    {
        real x;
        real y;
        real z;
    };

    typedef VERTDB_VEC3 vec3;

    struct vec3i
    {
        int_t x;
        int_t y;
        int_t z;
    };

    typedef VERTDB_PAIR<VERTDB_BONEID, VERTDB_BONEWEIGHT> bone_weight;
    typedef VERTDB_BUCKET<bone_weight> bone_weights;

    typedef VERTDB_VERTID vert_id;
    typedef VERTDB_BUCKET<vert_id> vert_connects;
    typedef VERTDB_MAP<vert_id, vert_id> vert_directory;

    const vert_id c_invalid_vert_id = -1;

    typedef VERTDB_MUTEX mutex_type;
    typedef VERTDB_LOCKGUARD<mutex_type> lock_type;
    typedef VERTDB_THREAD thread_type;
    typedef VERTDB_BUCKET<VERTDB_THREAD> thread_collection;
}
