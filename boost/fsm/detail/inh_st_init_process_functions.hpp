/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   inh_st_init_process_functions.hpp
 * \author Andrey Semashev
 * \date   28.01.2007
 * 
 * \brief  This header is the Boost.FSM library implementation, see the library documentation
 *         at http://www.boost.org/libs/state_machine/doc/state_machine.html. In this file
 *         a part of inherited_states class implementation resides.
 */

#define BOOST_FSM_STATE_TYPE() BOOST_PP_CAT(BOOST_PP_CAT(state, BOOST_PP_ITERATION()), _type)
            {
                // Form up the complete transitions list, including ones that are defined in the current state
                typedef mpl::joint_view<
                    typename BOOST_FSM_STATE_TYPE()::transitions_type_list,
                    typename StateMachineT::transitions_type_list
                > complete_transitions_type_list;
        
                // Find an applicable transition in transitions map
                typedef typename mpl::find_if<
                    complete_transitions_type_list,
                    applicable_transition_pred< BOOST_FSM_STATE_TYPE(), EventT >
                >::type transition_it_t;
                typedef typename is_same<
                    transition_it_t,
                    typename mpl::end< complete_transitions_type_list >::type
                >::type is_no_transition_found_t;
        
                // Fill actual function pointers
                state_machine_access::do_init_process_functions<
                    StateMachineT,
                    BOOST_FSM_STATE_TYPE(),
                    transition_it_t,
                    EventT,
                    ProcessFuncsT
                >(process_funcs, is_no_transition_found_t());
                ++process_funcs;
            }
#undef BOOST_FSM_STATE_TYPE
