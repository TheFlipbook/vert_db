#pragma once

#include "vert_db/vert_db.h"

namespace vd
{
    template<typename T>
    class transfer_resolver
    {
    public:
        typedef vert_db<T> db_type;
        typedef typename db_type::key_type key_type;
        typedef VERTDB_BUCKET<key_type> frontier_type;
        typedef typename frontier_type::iterator frontier_iterator;

        virtual ~transfer_resolver() {}

        virtual frontier_type resolve( const db_type &context, const frontier_iterator &begin, const frontier_iterator &end, db_type &results ) const = 0;
    };


    template<typename T=size_t>
    class transfer_db
    {
    public:
        typedef transfer_db<T> self_type;
        typedef vert_db<T> vert_db_type;
        typedef typename vert_db_type::key_type key_type;
        typedef transfer_resolver<T> resolver_type;
        typedef VERTDB_UNIQUE_PTR<resolver_type> resolver_handle;
        typedef VERTDB_BUCKET<resolver_handle> resolver_collection;
        typedef VERTDB_BUCKET<key_type> frontier_type;

        inline vert_db_type& vert_db()
        {
            return m_db;
        }

        inline const vert_db_type& vert_db() const
        {
            return m_db;
        }

        template<typename R, class ...Args>
        R& add_resolver( Args&& ...args )
        {
            auto ptr = VERTDB_MAKE_UNIQUE<R>( VERTDB_FORWARD<Args>( args )... );
            R& added = *ptr;
            m_resolvers.emplace_back( VERTDB_MOVE(ptr) );
            return added;
        }

        bool apply(vert_db_type &results)
        {
            auto start = results.begin();
            auto finish = results.end();
            frontier_type frontier( results.begin(), results.end() );

            for( auto& resolver : m_resolvers )
            {
                frontier = resolver->resolve( vert_db(), frontier.begin(), frontier.end(), results );
            }

            return true;
        }

    protected:
        vert_db_type m_db;
        resolver_collection m_resolvers;
    };
}
