/*
 * main_optimize.cpp
 * Date: 2013-01-28
 * Author: Karsten Ahnert (karsten.ahnert@gmx.de)
 */

#include "eval.hpp"

#include <gp/ga/ga1.hpp>
#include <gp/tree/linked_node_tree.hpp>
#include <gp/tree/generate_random_linked_tree.hpp>
#include <gp/operator/mutation.hpp>
#include <gp/operator/crossover.hpp>
#include <gp/stat/population_statistics.hpp>

#include <boost/make_shared.hpp>

#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#define tab "\t"

using namespace std;


template< class Rng , class Func >
void generate_test_data( vector_t &y , vector_t &x1 , vector_t &x2 , vector_t &x3 , size_t n , Rng &rng , Func f )
{
    std::normal_distribution< double > dist1( 0.0 , 1.0 ) , dist2( 0.0 , 1.0 ) , dist3( 0.0 , 1.0 );
    x1.resize( n ) ; x2.resize( n ) ; x3.resize( n ) ; y.resize( n ) ;
    for( size_t i = 0 ; i < n ; ++i )
    {
        x1[i] = dist1( rng );
        x2[i] = dist2( rng );
        x3[i] = dist3( rng );
        y[i] = f( x1[i] , x2[i] , x3[i] );
    }
}


int main( int argc , char *argv[] )
{
    typedef std::mt19937 rng_type ;
    typedef fitness_function::context_type context_type;
    typedef gp::genetic_evolver1< tree_type , fitness_function::context_type , std::mt19937 > evolver_type;



    rng_type rng;

    context_type c;
    generate_test_data( c.y , c.x1 , c.x2 , c.x3 , 10000 , rng ,
                        []( double x1 , double x2 , double x3 ) {
                            return x1 * x1 * x1 + 1.0 / 10.0 * x2 * x2 - 3.0 / 4.0 * ( x3 - 4.0 ) + 1.0 ; } );


    generators< rng_type > gen( rng );

    size_t population_size = 4000;
    double elite_rate = double( 2 ) / double( population_size );
    double mutation_rate = 0.2;
    double crossover_rate = 0.6;
    size_t min_tree_height = 4 , max_tree_height = 8;

    std::function< void( tree_type& ) > tree_generator;
    std::array< int , 3 > weights = {{ 2 * int( gen.gen0.num_symbols() ) ,
                                       int( gen.gen1.num_symbols() ) , 
                                       int( gen.gen2.num_symbols() ) }};
    tree_generator = make_tree_generator_binder( rng , gen.gen0 , gen.gen1 , gen.gen2 , min_tree_height , max_tree_height , weights );

    evolver_type evolver( elite_rate , mutation_rate , crossover_rate , rng );
    std::vector< double > fitness( population_size , 0.0 );
    std::vector< tree_type > population( population_size );


    evolver.fitness_function() = fitness_function();
    evolver.mutation_function() = gp::make_mutation_binder( rng , gen.gen0 , gen.gen1 , gen.gen2 );
    evolver.crossover_function() = gp::make_crossover_binder( rng , max_tree_height );
    evolver.random_individual_function() = tree_generator;


    // initialize population with random trees and evaluate fitness
    for( size_t i=0 ; i<population.size() ; ++i )
    {
        tree_generator( population[i] );
        fitness[i] = fitness_function()( population[i] , c );
//        cout << fitness[i] << " " << gp::simple( population[i] ) << "\n";
    }
    
    // for( size_t i=0 ; i<3 ; ++i )
    // {
    //     evolver.next_generation( population , fitness , c );

    //     std::vector< size_t > idx;
    //     auto iter = gp::sort_indices( fitness , idx );
    //     cout << fitness[ idx[0] ] << " : " << gp::simple( population[ idx[0] ] ) << "\n";
    // }

    return 0;
}