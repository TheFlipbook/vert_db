#pragma once

#include "vert_db_transfer.h"

namespace vd
{
    template<typename T>
    class transfer_resolver_base : public transfer_resolver<T>
    {
    public:
        transfer_resolver_base( item_flags to_set )
            : m_set( to_set )
        {
        }

        bool apply( const db_type &context, const key_type &context_key, db_type &results, const key_type &results_key ) const
        {
            auto to_set = results.make_def();
            to_set.flags |= m_set;
            context.gather( context_key, to_set );

            results.update_atomic( results_key, to_set );
            return true;
        }

    protected:
        item_flags m_set;
    };

    template<typename T>
    class transfer_resolver_threaded : public transfer_resolver_base<T>
    {
    public:
        typedef transfer_resolver_threaded<T> self_type;

        transfer_resolver_threaded( item_flags to_set )
            : transfer_resolver_base( to_set )
        {
        }

        virtual bool resolve_vert( const db_type &context, const key_type &key, db_type &results ) const = 0;

        frontier_type resolve( const db_type &context, const frontier_iterator &begin, const frontier_iterator &end, db_type &results ) const override
        {
            frontier_type next;

            processor_func runner{ context, *this, results };
            transfer_processor processor( runner, begin, end, next );
            processor.join();

            return next;
        }

    protected:
        struct processor_func
        {
            void operator()( const key_type &key, frontier_type &collector )
            {
                if( !m_resolver.resolve_vert( m_context, key, m_results ) )
                    collector.emplace_back( key );
            }

            const db_type &m_context;
            const self_type &m_resolver;
            db_type &m_results;
        };

        typedef threaded_processor<processor_func, typename frontier_type::iterator, frontier_type> transfer_processor;
    };

    template<typename T>
    class transfer_resolver_matched : public transfer_resolver_threaded<T>
    {
    public:
        transfer_resolver_matched( item_flags to_set, vd::real tolerance=1e-5 )
            : transfer_resolver_threaded(to_set)
            , m_tolerance( tolerance )
        {
        }

        bool resolve_vert( const db_type &context, const key_type &key, db_type &results ) const override
        {
            auto id = results.id( key );

            auto found_key = context.find_id( id );
            if( found_key == c_invalid_vert_id )
                return false;

            auto result_pos = results.position( key );
            auto distance = context.distance_to( result_pos, found_key );
            if( distance > m_tolerance )
                return false;

            return apply( context, found_key, results, key );
        }

    protected:
        typename db_type::scalar m_tolerance;
    };

    template<typename T>
    class transfer_resolver_position : public transfer_resolver_threaded<T>
    {
    public:
        transfer_resolver_position( item_flags to_set, vd::real tolerance = 1e-5 )
            : transfer_resolver_threaded( to_set )
            , m_tolerance( tolerance )
        {
        }

        bool resolve_vert( const db_type &context, const key_type &key, db_type &results ) const override
        {
            auto result_pos = results.position( key );
            auto found = context.find_position( result_pos, m_tolerance );

            if( found.empty() )
                return false;

            db_type::scalar best_dist = VERTDB_NUMERIC_LIMITS<db_type::scalar>::max();
            db_type::key_type best_key = found[0];

            for( const auto &found_key : found )
            {
                db_type::scalar dist = context.distance_to( result_pos, found_key );
                if( dist < best_dist )
                {
                    best_dist = dist;
                    best_key = found_key;
                }
            }

            if( best_key == c_invalid_vert_id )
                return false;

            return apply( context, best_key, results, key );
        }

    protected:
        typename db_type::scalar m_tolerance;
    };

    template<typename T>
    class transfer_resolver_uvw : public transfer_resolver_threaded<T>
    {
    public:
        transfer_resolver_uvw( item_flags to_set, vd::real tolerance = 1e-5 )
            : transfer_resolver_threaded( to_set )
            , m_tolerance( tolerance )
        {
        }

        bool resolve_vert( const db_type &context, const key_type &key, db_type &results ) const override
        {
            auto result_pos = results.uvw( key );
            auto found = context.find_uvw( result_pos, m_tolerance );

            if( found.empty() )
                return false;

            db_type::scalar best_dist = VERTDB_NUMERIC_LIMITS<db_type::scalar>::max();
            db_type::key_type best_key = found.first();

            for( const auto &found_key : found )
            {
                db_type::scalar dist = context.distance_to_uvw( results_pos, found_key );
                if( dist < best_dist )
                {
                    best_dist = dist;
                    best_key = found_key;
                }
            }

            if( best_key == c_invalid_vert_id )
                return false;

            return apply( context, best_key, results, key );
        }

    protected:
        typename db_type::scalar m_tolerance;
    };

    template<typename T>
    class transfer_resolver_gaussian : public transfer_resolver_threaded<T>
    {
    public:
        transfer_resolver_gaussian( item_flags to_set, vd::real radius, size_t weight_total=0, vd::real weight_clip=.05f, bool weight_normalize=true  )
            : transfer_resolver_threaded( to_set )
            , m_radius( radius )
            , m_weight_total(weight_total)
            , m_weight_clip(weight_clip)
            , m_weight_normalize(weight_normalize)
        {
        }

        bool resolve_vert( const db_type &context, const key_type &key, db_type &results ) const override
        {
            auto to_set = results.make_def();

            auto result_pos = results.position( key );

            auto verts = context.find_position_sorted( result_pos, m_radius );
            if( verts.empty() )
                return false;

            size_t best_key = verts.front();

            if( flag_is_set( m_set, k_item_id ) )
                to_set.set_id( context.id(best_key) );

            if( flag_is_set( m_set, k_item_position ) )
                to_set.set_position( result_pos );

            // TODO: filter normal similar to colors, ut guarantee unit length.
            if( flag_is_set( m_set, k_item_normal ) )
                to_set.set_normal( context.normal(best_key) );

            // TODO: Intelligently filter UVW somehow? Barycentric?
            if( flag_is_set( m_set, k_item_uvw ) )
                to_set.set_position( context.uvw( best_key ) );

            if( flag_is_set( m_set, k_item_color ) )
                to_set.set_color( context.sample_color(result_pos, m_radius) );

            if( flag_is_set( m_set, k_item_weights ) )
            {
                auto weights = context.find_weights( result_pos, m_radius, m_weight_total, m_weight_clip, m_weight_normalize );
                to_set.set_weights( weights );
            }

            // TODO: This can technically create one-way connects.  Maybe shouldn't?
            //       This should be consistent with ID query.
            //       Problem comes with multiple items sharing an ID.
            //       ID dupes should shake out during insert to results DB though.
            if( flag_is_set( m_set, k_item_connects ) )
            {
                to_set.set_connects( context.connects( best_key ) );
            }

            results.update_atomic( key, to_set );
            return true;
        }

    protected:
        typename db_type::scalar m_radius;
        size_t m_weight_total;
        vd::real m_weight_clip;
        bool m_weight_normalize;
    };


    template<typename T>
    class transfer_flood_fill : public transfer_resolver_base<T>
    {
        typedef transfer_flood_fill<T> self_type;
        typedef typename db_type::key_type key_type;
        typedef VERTDB_PAIR< key_type, db_item_def<T> > generation_item;
        typedef VERTDB_BUCKET< generation_item > generation_type;
        typedef typename db_type::def_type def_type;
        typedef VERTDB_BUCKET< def_type > def_collection;

    public:
        transfer_flood_fill( item_flags to_set, int depth=-1 )
            : transfer_resolver_base( to_set )
            , m_depth( depth )
        {
        }

        frontier_type resolve( const db_type &context, const frontier_iterator &begin, const frontier_iterator &end, db_type &results ) const override
        {
            frontier_type frontier( begin, end );
            frontier_type next;
            int generations = 0;

            while( !frontier.empty() )
            {
                next.clear();

                mutex_type generation_mutex;
                generation_type generation;

                processor_func runner{ results, *this, generation_mutex, generation };
                transfer_processor processor( runner, frontier.begin(), frontier.end(), next );
                processor.join();

                bool found_any = !generation.empty();

                for( const auto &item : generation )
                {
                    results.update( item.first, item.second );
                }

                // Only loop if there was meaningful progress
                if( found_any && ( frontier != next ) )
                    frontier = next;
                else
                    break;

                ++generations;
                if( ( m_depth > 0 ) && (generations >= m_depth) )
                    break;
            }

            return next;
        }

        bool resolve_vert(const key_type &key, const db_type &context, mutex_type &mutex, generation_type &generation) const
        {
            auto connects = context.connects( key );

            def_collection connect_defs;
            for( const auto &id : connects )
            {
                auto connect_def = context.make_def();
                if( context.gather( id, connect_def ) )
                {
                    if( flag_is_set( connect_def.flags, m_set ) )
                        connect_defs.emplace_back( connect_def );
                }
            }

            // Needs valid connections to flood fill
            if( connect_defs.empty() )
                return false;

            auto combined_def = combine_defs( connect_defs.begin(), connect_defs.end() );
            combined_def.flags = m_set;
            generation_item item( key, combined_def );

            {
                lock_type lock( mutex );
                generation.emplace_back( item );
            }

            return true;
        }

    protected:
        struct processor_func
        {
            void operator()( const key_type &key, frontier_type &collector )
            {
                if( !m_resolver.resolve_vert( key, m_context, m_generation_mutex, m_generation ) )
                    collector.emplace_back( key );
            }

            const db_type &m_context;
            const self_type &m_resolver;

            mutex_type &m_generation_mutex;
            generation_type &m_generation;
        };

        typedef threaded_processor<processor_func, typename frontier_type::iterator, frontier_type> transfer_processor;

        int m_depth;
    };
}
