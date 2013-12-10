/*
  gpcxx/util/parallel_algorithm.hpp

  Copyright 2013 Gerard Choinka

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef GPCXX_PARALLEL_ALGORITHM_HPP_DEFINED
#define GPCXX_PARALLEL_ALGORITHM_HPP_DEFINED

#include <vector>
#include <cmath>
#include <tuple>
#include <cassert>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/sliced.hpp>
#include <boost/range/adaptor/strided.hpp>


#ifndef GPCXX_USE_STD_THREAD_IMPLEMENTATION
#   define GPCXX_USE_STD_THREAD_IMPLEMENTATION 1
#endif

#if GPCXX_USE_STD_THREAD_IMPLEMENTATION == 1
    #include <thread>
    typedef std::thread gpcxx_thread_default_impl; 
    namespace gpcxx { namespace par { auto hardware_concurrency = std::thread::hardware_concurrency; } }
#else
    #include <boost/thread.hpp>
    typedef boost::thread gpcxx_thread_default_impl;
    namespace gpcxx { namespace par { auto hardware_concurrency = boost::thread::hardware_concurrency; } }
#endif

    
    
namespace gpcxx {

namespace par {
    
    
template < typename InputIterator, typename Distance >
InputIterator advance_with_copy ( InputIterator it, Distance n )
{
    std::advance( it, n );
    return it;
}

template<const size_t nThreads>
struct UseNThreads
{
    static const size_t value = nThreads;
};


template<
    typename single_pass_range1,
    typename output_iterator,
    typename unary_operation,
    typename thread_type = gpcxx_thread_default_impl
>
output_iterator transform( const single_pass_range1 & rng,
                           output_iterator out,
                           unary_operation fun,
                           size_t number_of_threads = gpcxx::par::hardware_concurrency()
                        )
{
    if ( number_of_threads == 1 )
        return boost::transform( rng, out, fun ); 

    auto & out_begin_iter = out;
    auto out_end_iter = advance_with_copy( out,  boost::size( rng ) );


    std::vector< thread_type >  worker{}; worker.reserve(number_of_threads);
    
    for (size_t i = 0; i < number_of_threads; ++i)
    {            
        auto outRngStrided = boost::make_iterator_range( advance_with_copy( out_begin_iter, i ),        out_end_iter ) 
            | boost::adaptors::strided( number_of_threads ); 
        auto inRngStrided =  boost::make_iterator_range( advance_with_copy( boost::begin( rng ), i ), boost::end( rng ) ) 
            | boost::adaptors::strided( number_of_threads ); 
        
        worker.emplace_back( [=](){ boost::transform( inRngStrided, boost::begin( outRngStrided ), fun ); } );
    }
    for ( auto& w: worker ) w.join();
    return out_begin_iter;
}




std::tuple< size_t, size_t > partition(size_t n_partition,
                                     size_t partition_number,
                                     size_t rng_size )
{
//     std::assert( n_partition > 0 );
//     std::assert( n_partition > partition_number );
    if( n_partition == 1 )
        return std::tuple<size_t, size_t>{ 0, rng_size };
            

    int remainder = rng_size % n_partition;
    int partition_size = rng_size / n_partition;
    
    if( remainder == 0 )
        return std::tuple<size_t, size_t>{ partition_size * partition_number, partition_size * (partition_number + 1) };

    if ( n_partition  == ( partition_number + 1 ))
        return std::tuple<size_t, size_t>{ partition_size * partition_number, rng_size };
    
    return std::tuple<size_t, size_t>{ partition_size * partition_number, partition_size * (partition_number + 1) };
}


template<
    typename single_pass_range1,
    typename single_pass_range2,
    typename output_iterator,
    typename unary_operation,
    typename thread_type = gpcxx_thread_default_impl
>
output_iterator transform( const single_pass_range1 & rng1,
                           const single_pass_range2 & rng2,
                           output_iterator out,
                           unary_operation fun,
                           size_t number_of_threads = gpcxx::par::hardware_concurrency()
                        )
{
//     std::assert( boost::size(rng1) == boost::size(rng2) );

    if ( number_of_threads == 1 )
        return boost::transform( rng1, rng2, out, fun ); 
    
    
    size_t rng1_size = boost::size( rng1 );
    
    if( rng1_size < number_of_threads )
        number_of_threads = rng1_size;
    
    size_t step_size = std::ceil(rng1_size / number_of_threads);
    

    auto & out_begin_iter = out;
    auto out_end_iter = advance_with_copy( out,  boost::size( rng1 ) );

    std::vector< thread_type >  worker{}; worker.reserve(number_of_threads);
    
    for (size_t i = 0; i < number_of_threads; ++i)
    {
        size_t rng_begin =     i*step_size;
        size_t rng_end   = (i+1)*step_size;
        auto sub_rng1 = rng1 | boost::adaptors::sliced( rng_begin, rng_end );
        auto sub_rng2 = rng2 | boost::adaptors::sliced( rng_begin, rng_end );
        auto out_iter = advance_with_copy( out, rng_begin );
        
        worker.emplace_back( [=](){ boost::transform( sub_rng1, sub_rng2, out_iter, fun ); } );
    }
    for ( auto& w: worker ) w.join();
    return out_begin_iter;
}


template<
    class SinglePassRange,
    class UnaryFunction,
    typename thread_type = gpcxx_thread_default_impl
>
UnaryFunction for_each(SinglePassRange& rng, UnaryFunction fun, const size_t number_of_threads = gpcxx::par::hardware_concurrency())
{
    if ( number_of_threads == 1 )
        return boost::for_each( rng, fun );
    
    std::vector< thread_type > worker{}; worker.reserve( number_of_threads );
    for (size_t i = 0; i < number_of_threads; ++i)
    {            
        auto rngStrided = boost::make_iterator_range( advance_with_copy( boost::begin( rng ), i ), boost::end( rng ) ) 
            | boost::adaptors::strided( number_of_threads ); 

        worker.emplace_back(  [=](){ boost::for_each( rngStrided, fun ); } );
    }
    for ( auto& w: worker ) w.join();
    
    return fun;
}

} // namespace par

} // namespace gpcxx


#endif // GPCXX_PARALLEL_ALGORITHM_HPP_DEFINED