/*!
 * (C) 2006 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   lightweight_call_once.hpp
 * \author Andrey Semashev
 * \date   07.02.2007
 * 
 * \brief  This header is the call_once concept light implementation.
 *
 * The basic usage is:
 *
 * <pre>
 * static call_once_trigger trigger = BOOST_LWCO_INIT;
 * call_once(trigger, fun);
 * </pre>
 *
 * Here "fun" is a nullary function object to be called. The call_once may throw an
 * std::exception-derived object to indicate errors.
 *
 * This implementation provides the following guarantees:
 * - The function object will be called only once, if it returns successfully (i.e. not with an
 *   exception), regardless of the number of threads and number of calls to call_once with the
 *   same trigger object.
 * - The function object execution will be complete after call_once successfully returns, whether
 *   or not the function object have been executed in the current thread.
 * - If the function object returns by throwing an exception, the function object is considered
 *   as never called. This may mean that the function object may be called again by the same or
 *   another thread.
 *
 * Limitations:
 * - Calls to call_once with the same trigger object must not be recursive
 * - Users should not try to use call_once_triggers in any way except to pass it to call_once
 * - A limited set of threading APIs is supported (WinAPI, including NT 6 innovations, POSIX, BeOS, MacOS)
 * - Calls to call_once may be expensive, even if no function object execution occurs, although
 *   all means were taken to make it as fast as possible
 *
 * Configuration:
 * - Define BOOST_LWCO_NO_EXPLICIT_MEMORY_BARRIERS macro to disable the use of explicit memory barriers
 *   and atomic operations to implement call_once if you experience problems. It may result in suboptimal and
 *   less fail-safe code on some platforms. Here lesser fail-safeness means that the implementation may have
 *   more points of acquiring system resources which may fail.
 * - Define BOOST_USE_WINDOWS_H on Windows platform to use native windows.h header instead
 *   of duplication of needed API parts in the implementation. When this option is enabled,
 *   all needed defines, such as WIN32_LEAN_AND_MEAN and _WIN32_WINNT, should be defined prior
 *   to this header inclusion.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1020)
#pragma once
#endif // _MSC_VER > 1020

#ifndef BOOST_LIGHTWEIGHT_CALL_ONCE_HPP_INCLUDED_
#define BOOST_LIGHTWEIGHT_CALL_ONCE_HPP_INCLUDED_

#include <boost/config.hpp>

#if defined(BOOST_HAS_THREADS)

// Use threading API specific implementation
#if defined(BOOST_HAS_WINTHREADS)
#include <boost/detail/lwco_win32.hpp>
#elif defined(BOOST_HAS_PTHREADS)
#include <boost/detail/lwco_pthreads.hpp>
#elif defined(BOOST_HAS_MPTASKS)
#include <boost/detail/lwco_macos.hpp>
#elif defined(BOOST_HAS_BETHREADS)
#include <boost/detail/lwco_beos.hpp>
#else
#error Unknown threading API detected
#endif

#else // defined(BOOST_HAS_THREADS)

// No threading detected, a single-thread implementation

namespace boost {

namespace detail {

namespace lw_call_once {

    //! The type used to trigger the execution
    typedef bool call_once_trigger;
    //! Initial trigger value
    #define BOOST_LWCO_INIT false

    //! The function calls functor fun only if the trigger is in init state
    template< typename T >
    inline void call_once(call_once_trigger& trigger, T const& fun)
    {
        if (trigger == BOOST_LWCO_INIT)
        {
            fun();
            trigger = true;
        }
    }

} // namespace lw_call_once

} // namespace detail

} // namespace boost

#endif // defined(BOOST_HAS_THREADS)

#endif // BOOST_LIGHTWEIGHT_CALL_ONCE_HPP_INCLUDED_
