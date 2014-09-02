/*!
 * (C) 2006 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   lwco_macos.hpp
 * \author Andrey Semashev
 * \date   10.02.2007
 * 
 * \brief  This header is the call_once concept light implementation for Mac OS threading API.
 */

#ifndef BOOST_LIGHTWEIGHT_CALL_ONCE_MACOS_HPP_INCLUDED_
#define BOOST_LIGHTWEIGHT_CALL_ONCE_MACOS_HPP_INCLUDED_

#include <unistd.h>
#include <libkern/OSAtomic.h>

namespace boost {

namespace detail {

namespace lw_call_once {

    //! The type used to trigger the execution
    typedef int32_t call_once_trigger;
    //! Initial trigger value
    #define BOOST_LWCO_INIT 0

    //! The function calls functor fun only if the trigger is in init state
    template< typename T >
    inline void call_once(call_once_trigger& trigger, T const& fun)
    {
        while (OSAtomicAdd32Barrier(0, &trigger) != 2)
        {
            if (OSAtomicCompareAndSwap32(0, 1, &trigger))
            {
                // Only single thread is passed here
                try
                {
                    fun();
                    OSAtomicIncrement32Barrier(&trigger);
                    break;
                }
                catch (...)
                {
                    OSAtomicAnd32(0, &trigger);
                    throw;
                }
            }
            else
            {
                // Other threads sleep here until the function object execution is completed
                while (OSAtomicOr32(0, &trigger) == 1)
                    usleep(500);
            }
        }
    }

} // namespace lw_call_once

} // namespace detail

} // namespace boost

#endif // BOOST_LIGHTWEIGHT_CALL_ONCE_MACOS_HPP_INCLUDED_
