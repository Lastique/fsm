/*!
 * (C) 2006 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   transition.hpp
 * \author Andrey Semashev
 * \date   23.12.2006
 * 
 * \brief  This header is the Boost.FSM library implementation, see the library documentation
 *         at http://www.boost.org/libs/state_machine/doc/state_machine.html. In this file
 *         transitions map support is implemented.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1020)
#pragma once
#endif // _MSC_VER > 1020

#ifndef BOOST_FSM_TRANSITION_HPP_INCLUDED_
#define BOOST_FSM_TRANSITION_HPP_INCLUDED_

#include <boost/mpl/and.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/fsm/detail/prologue.hpp>

namespace boost {

namespace fsm {

//! A tag that represents any state type
struct any_state;

//! Base class for transitions
template< typename NextStateT >
struct basic_transition
{
    //! The function actually performs the transition
    template< typename CurrentStateT, typename EventT >
    static BOOST_FSM_FORCEINLINE void transit(CurrentStateT& state, EventT const&)
    {
        state.BOOST_NESTED_TEMPLATE switch_to< NextStateT >();
    }
};

/*!
*    \brief A simple transition rule implementation
*
*    The rule allows the transition to state NextStateT if the
*    current state is CurrentStateT and the event to be processed is EventT.
*/
template< typename CurrentStateT, typename EventT, typename NextStateT >
struct transition : public basic_transition< NextStateT >
{
    //! Static predicate that checks if the rule is applicable
    template< typename StateT, typename EvtT >
    struct is_applicable :
        public mpl::and_<
            is_same< CurrentStateT, StateT >,
            is_same< EvtT, EventT >
        >
    {
    };
};

/*!
*    \brief A simple transition rule implementation
*
*    Specialization for any_state
*/
template< typename EventT, typename NextStateT >
struct transition< any_state, EventT, NextStateT > : public basic_transition< NextStateT >
{
    //! Static predicate that checks if the rule is applicable
    template< typename StateT, typename EvtT >
    struct is_applicable :
        public is_same< EvtT, EventT >
    {
    };
};

} // namespace fsm

} // namespace boost

#endif // BOOST_FSM_TRANSITION_HPP_INCLUDED_
