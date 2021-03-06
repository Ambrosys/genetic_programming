/*
 * test/canonic/remove_double_inverse_functions.cpp
 * Date: 2015-09-17
 * Author: Karsten Ahnert (karsten.ahnert@gmx.de)
 * Copyright: Karsten Ahnert
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include "canonic_test_trees.hpp"
#include "../common/test_functions.hpp"
#include <gpcxx/canonic/remove_double_inverse_functions.hpp>

#include <gtest/gtest.h>

#define TESTNAME remove_double_inverse_functions_tests

using node_type = canonic_test_trees::node_type;
using tree_type = canonic_test_trees::tree_type;
using rule = std::function< gpcxx::rule_result( tree_type& , tree_type::cursor ) >;
using rule_container = std::vector< rule >;
using algebras_type = gpcxx::algebras< node_type >;
using group_type = algebras_type::group_type;

    
TEST( TESTNAME , test1 )
{
    algebras_type algebras {};
    algebras.add_abelian_group( group_type {
        node_type::make_binary_operation( gpcxx::plus_func {} , "+" ) ,
        node_type::make_constant_terminal( gpcxx::double_terminal<> { 0.0 } , "0" ) ,
        node_type::make_binary_operation( gpcxx::minus_func {} , "-" ) ,
        node_type::make_identity_operation( gpcxx::unary_minus_func {} , "um" )
        } );

    auto t = canonic_test_trees::test_tree5();
    rule_container rules { gpcxx::make_remove_double_inverse_functions( algebras ) };
    gpcxx::transform_tree( rules , t );

    EXPECT_EQ( t.size() , size_t( 1 ) );
    test_cursor( t.root() , "x" , 0 , 1 , 0 );
}

TEST( TESTNAME , test2 )
{
    algebras_type algebras {};
    algebras.add_abelian_group( group_type {
        node_type::make_binary_operation( gpcxx::plus_func {} , "+" ) ,
        node_type::make_constant_terminal( gpcxx::double_terminal<> { 0.0 } , "0" ) ,
        node_type::make_binary_operation( gpcxx::minus_func {} , "-" ) ,
        node_type::make_identity_operation( gpcxx::unary_minus_func {} , "um" )
        } );

    auto t = canonic_test_trees::test_tree6();
    rule_container rules { gpcxx::make_remove_double_inverse_functions( algebras ) };
    gpcxx::transform_tree( rules , t );

    EXPECT_EQ( t.size() , size_t( 2 ) );
    test_cursor( t.root() , "sin" , 1 , 2 , 0 );
    test_cursor( t.root().children(0) , "x" , 0 , 1 , 1 );
}
