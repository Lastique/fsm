/*!
 * (C) 2006 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   inherited_states.hpp
 * \author Andrey Semashev
 * \date   28.01.2007
 * 
 * \brief  This header is the Boost.FSM library implementation, see the library documentation
 *         at http://www.boost.org/libs/state_machine/doc/state_machine.html. In this file
 *         a part of inherited_states class implementation resides.
 */

#define BOOST_FSM_STATE_IMPL_BASE(z, iter, data)\
    public state_impl< typename mpl::deref< typename mpl::advance_c< StatesListIterT, BOOST_PP_INC(iter) >::type >::type, StatesListT, RetValT >

    //! An internal class that recursively inherits all states
    template< typename StatesListT, typename RetValT, typename StatesListIterT >
    struct BOOST_FSM_NO_VTABLE inherited_states< StatesListT, RetValT, StatesListIterT, BOOST_PP_ITERATION() > :
        public state_impl< typename mpl::deref< StatesListIterT >::type, StatesListT, RetValT >
        BOOST_PP_ENUM_TRAILING(BOOST_PP_DEC(BOOST_PP_ITERATION()), BOOST_FSM_STATE_IMPL_BASE, ~)
    {

#undef BOOST_FSM_STATE_IMPL_BASE

        //! State machine return type
        typedef RetValT return_type;

        //! State type
        typedef typename mpl::deref< StatesListIterT >::type state1_type;
        //! State super-class
        typedef state_impl< state1_type, StatesListT, RetValT > state1_impl_type;

#define BOOST_FSM_STATE_TYPEDEFS(z, iter, data)\
    typedef typename mpl::deref< typename mpl::advance_c< StatesListIterT, iter >::type >::type BOOST_PP_CAT(BOOST_PP_CAT(state, BOOST_PP_INC(iter)), _type);\
    typedef state_impl< BOOST_PP_CAT(BOOST_PP_CAT(state, BOOST_PP_INC(iter)), _type), StatesListT, RetValT > BOOST_PP_CAT(BOOST_PP_CAT(state, BOOST_PP_INC(iter)), _impl_type);

        BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_ITERATION(), BOOST_FSM_STATE_TYPEDEFS, ~)

#undef BOOST_FSM_STATE_TYPEDEFS

        //! The method recursively invokes on_reset handlers for all states
        BOOST_FSM_FORCEINLINE void on_reset()
        {
#define BOOST_PP_ITERATION_LIMITS (1, BOOST_PP_ITERATION())
#define BOOST_PP_FILENAME_2 <boost/fsm/detail/inh_st_on_reset.hpp>
#include BOOST_PP_ITERATE()
        }

        /*!
        *    \brief The method fills the state information array
        *    \param pRoot reinterpret_cast'ed pointer to state_machine_root of this state machine
        *    \param pStateInfo pointer to state info to fill
        */
        BOOST_FSM_FORCEINLINE void init_states_info(const char* pRoot, volatile state_info* pStateInfo) const
        {
            BOOST_FSM_ASSUME(this != NULL);
#define BOOST_PP_ITERATION_LIMITS (1, BOOST_PP_ITERATION())
#define BOOST_PP_FILENAME_2 <boost/fsm/detail/inh_st_init_states_info.hpp>
#include BOOST_PP_ITERATE()
        }

        //! The method recursively initializes the dispatching map
        template< typename StateMachineT, typename EventT, typename ProcessFuncsT >
        static BOOST_FSM_FORCEINLINE void init_process_functions(ProcessFuncsT* process_funcs)
        {
#define BOOST_PP_ITERATION_LIMITS (1, BOOST_PP_ITERATION())
#define BOOST_PP_FILENAME_2 <boost/fsm/detail/inh_st_init_process_functions.hpp>
#include BOOST_PP_ITERATE()
        }
    };
