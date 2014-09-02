/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   inh_st_init_states_info.hpp
 * \author Andrey Semashev
 * \date   28.01.2007
 * 
 * \brief  This header is the Boost.FSM library implementation, see the library documentation
 *         at http://www.boost.org/libs/state_machine/doc/state_machine.html. In this file
 *         a part of inherited_states class implementation resides.
 */

#define BOOST_FSM_STATE_TYPE() BOOST_PP_CAT(BOOST_PP_CAT(state, BOOST_PP_ITERATION()), _type)
            {
                // Let's calculate the difference between the pointer to state_machine_root and the
                // pointer to state_root of the current state. This looks like a hack but it actually
                // conforms to C++ Standard. All operations are made on the valid object of state machine
                // and the Standard guarantees that binary layout of the object will not change neither
                // on constructing copies of object nor on assignment (which implies it will never change).
                // And since sizeof(char) == 1 we calculate the pointer difference with maximum percision.
                register const char* pState = reinterpret_cast< const char* >(
                    static_cast< const state_root* >(static_cast< const BOOST_FSM_STATE_TYPE()* >(this)));
        
                // Fill the state info and go on filling for other states
                pStateInfo->Shift = pState - pRoot;
                pStateInfo->pTypeInfo = &typeid(BOOST_FSM_STATE_TYPE());
                pStateInfo->pGetStateName = (state_info::get_state_name_fun_t)&BOOST_FSM_STATE_TYPE()::get_state_name;
                ++pStateInfo;
            }
#undef BOOST_FSM_STATE_TYPE
