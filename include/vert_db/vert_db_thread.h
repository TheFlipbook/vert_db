#pragma once

#include "vert_db_types.h"

namespace vd
{
    template<typename Func, typename In, typename Out>
    class threaded_processor
    {
    public:
        threaded_processor( Func &func, const In &begin, const In &end, Out &results, size_t thread_count = 0 )
            : m_result_mutex()
            , m_threads()
            , m_func(func)
            , m_results(results)
        {
            if( thread_count == 0 )
                thread_count = thread_type::hardware_concurrency();
        
            if( thread_count == 0 )
                thread_count = 1;
        
            size_t item_count = VERTDB_ITERATOR_DISTANCE( begin, end );
            if( item_count < thread_count )
                thread_count = item_count;
        
            if( item_count > 0 )
            {
                size_t stride = item_count / thread_count;
        
                for( size_t i = 0; i < thread_count; ++i )
                {
                    In start = begin;
                    In stop = begin;
                    VERTDB_ITERATOR_ADVANCE( start, stride*i );
        
                    if( i == ( thread_count - 1 ) )
                        stop = end;
                    else
                        VERTDB_ITERATOR_ADVANCE( stop, stride*( i + 1 ) );
        
                    m_threads.emplace_back( [=] { this->thread_func( start, stop ); } );
                }
            }
        }
        
        void thread_func( const In &begin, const In &end )
        {
            Out thread_results;
            
            for( auto it = begin; it != end; ++it )
            {
                m_func( *it, thread_results );
            }
        
            if( !thread_results.empty() )
            {
                lock_type guard( m_result_mutex );
                m_results.insert( m_results.end(), thread_results.begin(), thread_results.end() );
            }
        }

        void join()
        {
            for( auto &thread : m_threads )
            {
                thread.join();
            }
        }

    protected:
        mutex_type m_result_mutex;
        thread_collection m_threads;
        Func &m_func;
        Out &m_results;
    };
};