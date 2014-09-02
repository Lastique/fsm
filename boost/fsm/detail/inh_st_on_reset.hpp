/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   inh_st_on_reset.hpp
 * \author Andrey Semashev
 * \date   28.01.2007
 * 
 * \brief  This header is the Boost.FSM library implementation, see the library documentation
 *         at http://www.boost.org/libs/state_machine/doc/state_machine.html. In this file
 *         a part of inherited_states class implementation resides.
 */

#define BOOST_FSM_STATE_IMPL_TYPE() BOOST_PP_CAT(BOOST_PP_CAT(state, BOOST_PP_ITERATION()), _impl_type)
            try
            {
                BOOST_FSM_STATE_IMPL_TYPE()::on_reset();
            }
            catch(...)
            {
            }
#undef BOOST_FSM_STATE_IMPL_TYPE
