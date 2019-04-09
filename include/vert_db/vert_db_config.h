#pragma once

// Default floating point type
#ifndef VERTDB_SCALAR
#define VERTDB_SCALAR double
#endif

// Default integer type (for indexing vertices)
#ifndef VERTDB_INTEGER
#define VERTDB_INTEGER int
#endif

// Numeric Limits template equivalent for matching to VERTDB_SCALAR and VERTDB_Integer
#ifndef VERTDB_NUMERIC_LIMITS
#include <limits>
#define VERTDB_NUMERIC_LIMITS std::numeric_limits
#endif

// Scale value on VERTDB_NUMERIC_LIMITS's epsilon for near-equals calculations
#ifndef VERTDB_EPSILON_SCALE
#define VERTDB_EPSILON_SCALE 1000
#endif

// Pi constant in a form that VERTDB_SCALAR can consume
#ifndef VERTDB_PI
    #ifndef _USE_MATH_DEFINES
        #define _USE_MATH_DEFINES
    #endif
#include <math.h>
#define VERTDB_PI M_PI
#endif

// Main position/normal storage type
//   By Default uses VERTDB_SCALAR
#ifndef VERTDB_VEC3
#define VERTDB_VEC3 vd::vec3_internal
#endif

// Container for mapping keys/values such as in VERTDB_MAP
#ifndef VERTDB_PAIR
#include <utility>
#define VERTDB_PAIR std::pair
#endif

// Type for indexing into both VERTDB_MAP as well as VERTDB_DATA_STORAGE
#ifndef VERTDB_VERTID
#define VERTDB_VERTID size_t
#endif

// Container simple unique storage
#ifndef VERTDB_SET
#include <unordered_set>
#define VERTDB_SET std::unordered_set
#endif

// Container large key-value storage
#ifndef VERTDB_MAP
#include <unordered_map>
#define VERTDB_MAP std::unordered_map
#endif

// Container for large blocks of linear storage (such as user data)
#ifndef VERTDB_DATA_STORAGE
#include <vector>
#define VERTDB_DATA_STORAGE std::vector
#endif

// Container for storage of a small number of items
//  Often linearly searched
#ifndef VERTDB_BUCKET
#include <vector>
#define VERTDB_BUCKET std::vector
#endif

// Container for storage of a small number of items
//  Often linearly searched
#ifndef VERTDB_BUCKET_SORTER
#include <algorithm>
#define VERTDB_BUCKET_SORTER std::sort
#endif

// Default type used to uniquely identify bones for skin weight operations
#ifndef VERTDB_BONEID
#include <string>
#define VERTDB_BONEID std::string
#endif

// Default type used for storing bone weights
#ifndef VERTDB_BONEWEIGHT
#define VERTDB_BONEWEIGHT VERTDB_SCALAR
#endif

// Template for move semantics
#ifndef VERTDB_MOVE
#include <memory>
#define VERTDB_MOVE std::move
#endif

// Type that acts as an owning pointer
#ifndef VERTDB_UNIQUE_PTR
#include <memory>
#define VERTDB_UNIQUE_PTR std::unique_ptr
#endif

// Create a VERTDB_UNIQUE_PTR
#ifndef VERTDB_MAKE_UNIQUE
#include <memory>
#define VERTDB_MAKE_UNIQUE std::make_unique
#endif

#ifndef VERTDB_ITERATOR_ADVANCE
#include <iterator>
#define VERTDB_ITERATOR_ADVANCE std::advance
#endif

#ifndef VERTDB_ITERATOR_DISTANCE
#include <iterator>
#define VERTDB_ITERATOR_DISTANCE std::distance
#endif

#ifndef VERTDB_ITERATOR_TRAITS
#include <iterator>
#define VERTDB_ITERATOR_TRAITS std::iterator_traits
#endif

#ifndef VERTDB_FORWARD
#include <utility>
#define VERTDB_FORWARD std::forward
#endif

#ifndef VERTDB_IOTA
#include <numeric>
#define VERTDB_IOTA std::iota
#endif

// Type that can wrap a reference through type deduction to get to VERTDB_TREAD invoke
#ifndef VERTDB_REF
#include <memory>
#define VERTDB_REF std::ref
#endif

// Type that can be constructed to spawn a thread and join()'d to finish
#ifndef VERTDB_THREAD
#include <thread>
#define VERTDB_THREAD std::thread
#endif

// Type that can be constructed and lock for mutual access to a resouce
#ifndef VERTDB_MUTEX
#include <mutex>
#define VERTDB_MUTEX std::mutex
#endif

// Type that can RAII wrap a VERTDB_MUTEX to secure access
#ifndef VERTDB_LOCKGUARD
#define VERTDB_LOCKGUARD std::lock_guard
#endif
