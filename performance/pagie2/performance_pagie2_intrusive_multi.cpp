/*
 * main_optimize.cpp
 * Date: 2013-01-28
 * Author: Karsten Ahnert (karsten.ahnert@gmx.de)
 */

#define FUSION_MAX_VECTOR_SIZE 20

#include <gpcxx/tree/intrusive_tree.hpp>
#include <gpcxx/tree/basic_intrusive_node.hpp>
#include <gpcxx/tree/basic_named_intrusive_node.hpp>
#include <gpcxx/tree/intrusive_func.hpp>

#include <gpcxx/generate/uniform_symbol.hpp>
#include <gpcxx/generate/ramp.hpp>
#include <gpcxx/operator/mutation.hpp>
#include <gpcxx/operator/simple_mutation_strategy.hpp>
#include <gpcxx/operator/random_selector.hpp>
#include <gpcxx/operator/tournament_selector.hpp>
#include <gpcxx/operator/crossover.hpp>
#include <gpcxx/operator/one_point_crossover_strategy.hpp>
#include <gpcxx/operator/reproduce.hpp>
#include <gpcxx/eval/static_eval.hpp>
#include <gpcxx/eval/regression_fitness.hpp>
#include <gpcxx/evolve/static_pipeline.hpp>
#include <gpcxx/io/best_individuals.hpp>
#include <gpcxx/stat/population_statistics.hpp>
#include <gpcxx/app/timer.hpp>
#include <gpcxx/app/normalize.hpp>

#include <boost/fusion/include/make_vector.hpp>
#include <boost/container/deque.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/strided.hpp>
#include <thread>
//#include <libs/coroutine/example/c++11/tree.h>

#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <functional>
#include <boost/thread.hpp>

#define tab "\t"

namespace fusion = boost::fusion;

typedef double value_type;
typedef gpcxx::regression_training_data< value_type , 3 > trainings_data_type;
typedef std::mt19937 rng_type ;
typedef std::array< value_type , 3 > eval_context_type;
typedef std::vector< value_type > fitness_type;
typedef gpcxx::basic_named_intrusive_node< double , eval_context_type > node_type;
typedef gpcxx::intrusive_tree< node_type > tree_type;
typedef std::vector< tree_type > population_type;


struct evaluator
{
    typedef eval_context_type context_type;
    typedef double value_type;

    value_type operator()( tree_type const& t , context_type const& c ) const
    {
        return t.root()->eval( c );
    }
};

template< typename F >
void generate_test_data( trainings_data_type &data, double rmin , double rmax , double stepsize , F f )
{
    data.x[0].clear(); data.x[1].clear(); data.x[2].clear(); data.y.clear();
    
    for( double xx = rmin ; xx <= rmax ; xx += stepsize )
    {
        for( double yy = rmin ; yy <= rmax ; yy += stepsize )
        {
            for( double zz = rmin ; zz <= rmax ; zz += stepsize )
            {
                data.x[0].push_back( xx );
                data.x[1].push_back( yy );
                data.x[2].push_back( zz );
                data.y.push_back( f( xx , yy , zz ) );
            }
        }
    }
}



namespace multi {

template < typename InputIterator, typename Distance >
InputIterator advanceWithCopy ( InputIterator it, Distance n )
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
    typename SinglePassRange1,
    typename OutputIterator,
    typename UnaryOperation,
    typename NThreadsType,
    typename ThreadType = boost::thread
>
OutputIterator transform( const SinglePassRange1 & rng,
                          OutputIterator out,
                          UnaryOperation fun,
                          NThreadsType nth = UseNThreads<1>{}
                        )
{
    const size_t nThreads = NThreadsType::value;
    if ( nThreads == 1 )
        return boost::transform( rng, out, fun ); 

    auto & outBeginIter = out;
    auto outEndIter = advanceWithCopy( out,  boost::size( rng ) );

    ThreadType worker[ nThreads ];
    for (size_t i = 0; i < nThreads; ++i)
    {            
        auto outRngStrided = boost::make_iterator_range( advanceWithCopy( outBeginIter, i ),        outEndIter ) 
            | boost::adaptors::strided( nThreads ); 
        auto inRngStrided =  boost::make_iterator_range( advanceWithCopy( boost::begin( rng ), i ), boost::end( rng ) ) 
            | boost::adaptors::strided( nThreads ); 
        
        worker[ i ] = std::move( ThreadType( [=](){ boost::transform( inRngStrided, boost::begin( outRngStrided ), fun ); } ) );
    }
    for ( auto& w: worker ) w.join();
    return outBeginIter;
}


}
namespace pl = std::placeholders;

int main( int argc , char *argv[] )
{
    rng_type rng;
    
    trainings_data_type c;
    generate_test_data( c , -5.0 , 5.0 + 0.1 , 0.4 , []( double x1 , double x2 , double x3 ) {
                        return  1.0 / ( 1.0 + pow( x1 , -4.0 ) ) + 1.0 / ( 1.0 + pow( x2 , -4.0 ) ) + 1.0 / ( 1.0 + pow( x3 , -4.0 ) ); } );
    gpcxx::normalize( c.y );
    

    std::ofstream fout1( "testdata.dat" );
    for( size_t i=0 ; i<c.x[0].size() ; ++i )
        fout1 << c.y[i] << " " << c.x[0][i] << " " << c.x[1][i] << " " << c.x[2][i] << "\n";
    fout1.close();
    
   
    typedef gpcxx::static_pipeline< population_type , fitness_type , rng_type > evolver_type;

    gpcxx::uniform_symbol< node_type > terminal_gen { std::vector< node_type >{
        node_type { gpcxx::array_terminal< 0 >{} , "x" } ,
        node_type { gpcxx::array_terminal< 1 >{} , "y" } ,
        node_type { gpcxx::array_terminal< 2 >{} , "z" } } };
    gpcxx::uniform_symbol< node_type > unary_gen { std::vector< node_type >{
        node_type { gpcxx::sin_func{} , "s" } ,
        node_type { gpcxx::cos_func{} , "c" } ,
        node_type { gpcxx::exp_func{} , "e" } ,
        node_type { gpcxx::log_func{} , "l" } } };
    gpcxx::uniform_symbol< node_type > binary_gen { std::vector< node_type > {
        node_type { gpcxx::plus_func{} , "+" } ,
        node_type { gpcxx::minus_func{} , "-" } ,
        node_type { gpcxx::multiplies_func{} , "*" } ,
        node_type { gpcxx::divides_func{} , "/" }    
    } };
    
    size_t population_size = 32;
    size_t generation_size = 20;
    double number_elite = 1;
    double mutation_rate = 0.0;
    double crossover_rate = 0.6;
    double reproduction_rate = 0.3;
    size_t min_tree_height = 8 , max_tree_height = 8;
    size_t tournament_size = 15;


    std::array< double , 3 > weights = {{ 2.0 * double( terminal_gen.num_symbols() ) ,
                                       double( unary_gen.num_symbols() ) ,
                                       double( binary_gen.num_symbols() ) }};
    auto tree_generator = gpcxx::make_ramp( rng , terminal_gen , unary_gen , binary_gen , min_tree_height , max_tree_height , 0.5 , weights );
    

    evolver_type evolver( number_elite , mutation_rate , crossover_rate , reproduction_rate , rng );
    std::vector< double > fitness( population_size , 0.0 );
    std::vector< tree_type > population( population_size );


    evolver.mutation_function() = gpcxx::make_mutation(
        gpcxx::make_simple_mutation_strategy( rng , terminal_gen , unary_gen , binary_gen ) ,
        gpcxx::make_tournament_selector( rng , tournament_size ) );
    evolver.crossover_function() = gpcxx::make_crossover( 
        gpcxx::make_one_point_crossover_strategy( rng , max_tree_height ) ,
        gpcxx::make_tournament_selector( rng , tournament_size ) );
    evolver.reproduction_function() = gpcxx::make_reproduce( gpcxx::make_tournament_selector( rng , tournament_size ) );
    
    gpcxx::timer timer;
    auto fitness_f = gpcxx::make_regression_fitness( evaluator() );

    // initialize population with random trees and evaluate fitness
    timer.restart();
    for( size_t i=0 ; i<population.size() ; ++i )
    {
        tree_generator( population[i] );
        //fitness[i] = fitness_f( population[i] , c );
    }
    multi::transform( population, fitness.begin(), [&]( tree_type const &t ) { return fitness_f( t , c ); } , multi::UseNThreads<2>{});
      
    std::cout << "Generation time " << timer.seconds() << std::endl;
    std::cout << "Best individuals" << std::endl << gpcxx::best_individuals( population , fitness , 1 , 10 ) << std::endl;
    std::cout << "Statistics : " << gpcxx::calc_population_statistics( population ) << std::endl;
    std::cout << std::endl << std::endl;

    timer.restart();
    for( size_t i=0 ; i<generation_size ; ++i )
    {
        gpcxx::timer iteration_timer;
        iteration_timer.restart();
        evolver.next_generation( population , fitness );
        double evolve_time = iteration_timer.seconds();
        iteration_timer.restart();
        multi::transform( population, fitness.begin(), [&]( tree_type const &t ) { return fitness_f( t , c ); } , multi::UseNThreads<2>{});
        double eval_time = iteration_timer.seconds();
        
        std::cout << gpcxx::indent( 1 ) << "Iteration " << i << std::endl;
        std::cout << gpcxx::indent( 1 ) << "Evolve time " << evolve_time << std::endl;
        std::cout << gpcxx::indent( 1 ) << "Eval time " << eval_time << std::endl;
        std::cout << gpcxx::indent( 1 ) << "Best individuals" << std::endl << gpcxx::best_individuals( population , fitness , 2 , 10 ) << std::endl;
        std::cout << gpcxx::indent( 1 ) << "Statistics : " << gpcxx::calc_population_statistics( population ) << std::endl << std::endl;
    }
    std::cout << "Overall time : " << timer.seconds() << std::endl;

    return 0;
}