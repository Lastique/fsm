/*!
 * (C) 2006 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   lwco_win32.hpp
 * \author Andrey Semashev
 * \date   07.02.2007
 * 
 * \brief  This header is the call_once concept light implementation for Win32 threading API.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1020)
#pragma once
#endif // _MSC_VER > 1020

#ifndef BOOST_LIGHTWEIGHT_CALL_ONCE_WIN32_HPP_INCLUDED_
#define BOOST_LIGHTWEIGHT_CALL_ONCE_WIN32_HPP_INCLUDED_

// Windows Vista and later API support
#if (!defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0600)) && !defined(BOOST_LWCO_NO_NT6_WINAPI_SUPPORT)
#   define BOOST_LWCO_NO_NT6_WINAPI_SUPPORT 1
#endif // !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0600)

#ifdef BOOST_USE_WINDOWS_H
#   include <windows.h>
#elif defined(_MSC_VER)
#   pragma comment(lib, "kernel32")
#endif // BOOST_USE_WINDOWS_H

#if !defined(BOOST_LWCO_NO_EXPLICIT_MEMORY_BARRIERS)
#   include <boost/detail/memory_barrier.hpp>
#   ifdef BOOST_NO_MEMORY_BARRIERS
#       define BOOST_LWCO_NO_MEMORY_BARRIERS 1
#   endif
#else
#   define BOOST_LWCO_NO_MEMORY_BARRIERS 1
#endif

#if defined(BOOST_LWCO_NO_NT6_WINAPI_SUPPORT) || defined(BOOST_LWCO_NO_MEMORY_BARRIERS)
#   include <boost/detail/interlocked.hpp>
#endif

namespace boost {

namespace detail {

namespace lw_call_once {

#ifndef BOOST_USE_WINDOWS_H

    // Mimic some parts of WinAPI

#   ifdef BOOST_LWCO_NO_NT6_WINAPI_SUPPORT

    extern "C" {

        __declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);

    } // extern "C"

#       define BOOST_LWCO_INTERLOCKED_CMP_XCHG(x, y, z) BOOST_INTERLOCKED_COMPARE_EXCHANGE(x, y, z)
#       define BOOST_LWCO_INTERLOCKED_XCHG(x, y) BOOST_INTERLOCKED_EXCHANGE(x, y)
#       define BOOST_LWCO_INTERLOCKED_INC(x) BOOST_INTERLOCKED_INCREMENT(x)

#   else // BOOST_LWCO_NO_NT6_WINAPI_SUPPORT

    typedef struct _RTL_SRWLOCK { void* Ptr; } SRWLOCK;

    extern "C" {

        __declspec(dllimport) void __stdcall ReleaseSRWLockExclusive(SRWLOCK* SRWLock);
        __declspec(dllimport) void __stdcall AcquireSRWLockExclusive(SRWLOCK* SRWLock);

    } // extern "C"

#       ifdef BOOST_LWCO_NO_MEMORY_BARRIERS
#           define BOOST_LWCO_INTERLOCKED_CMP_XCHG(x, y, z) BOOST_INTERLOCKED_COMPARE_EXCHANGE(x, y, z)
#       endif

#   endif // BOOST_LWCO_NO_NT6_WINAPI_SUPPORT

#else // !BOOST_USE_WINDOWS_H

#   if defined(BOOST_LWCO_NO_NT6_WINAPI_SUPPORT) || defined(BOOST_LWCO_NO_MEMORY_BARRIERS)

    //! An additional traits to adjust InterlockedCompareExchange argument types that differ between compilers
    template< typename RetValT, typename ICEArgT1, typename ICEArgT2, typename ICEArgT3, typename T1, typename T2, typename T3 >
    inline RetValT interlocked_compare_exchange(RetValT (__stdcall *ice)(ICEArgT1, ICEArgT2, ICEArgT3), T1 arg1, T2 arg2, T3 arg3)
    {
        return ice((ICEArgT1)arg1, (ICEArgT2)arg2, (ICEArgT3)arg3);
    }

#       define BOOST_LWCO_INTERLOCKED_CMP_XCHG(x, y, z) interlocked_compare_exchange(&InterlockedCompareExchange, x, y, z)

#   endif // defined(BOOST_LWCO_NO_NT6_WINAPI_SUPPORT) || defined(BOOST_LWCO_NO_MEMORY_BARRIERS)

#   ifdef BOOST_LWCO_NO_NT6_WINAPI_SUPPORT

    //! An additional traits to adjust InterlockedCompareExchange argument types that differ between compilers
    template< typename RetValT, typename IEArgT1, typename IEArgT2, typename T1, typename T2 >
    inline RetValT interlocked_exchange(RetValT (__stdcall *ie)(IEArgT1, IEArgT2), T1 arg1, T2 arg2)
    {
        return ie((IEArgT1)arg1, (IEArgT2)arg2);
    }
    //! An additional traits to adjust InterlockedIncrement argument types that differ between compilers
    template< typename RetValT, typename IIArgT1, typename T1 >
    inline RetValT interlocked_increment(RetValT (__stdcall *ii)(IIArgT1), T1 arg1)
    {
        return ii((IIArgT1)arg1);
    }

#       define BOOST_LWCO_INTERLOCKED_XCHG(x, y) interlocked_exchange(&InterlockedExchange, x, y)
#       define BOOST_LWCO_INTERLOCKED_INC(x) interlocked_increment(&InterlockedIncrement, x)

#   endif // BOOST_LWCO_NO_NT6_WINAPI_SUPPORT

#endif // !BOOST_USE_WINDOWS_H

#ifdef BOOST_LWCO_NO_NT6_WINAPI_SUPPORT

    //! The type used to trigger the execution
    typedef long call_once_trigger;
    //! Initial trigger value
#   define BOOST_LWCO_INIT 0

    //! The function calls functor fun only if the trigger is in init state
    template< typename T >
    inline void call_once(call_once_trigger volatile& trigger, T const& fun)
    {
        // All interlocked functions act as memory barriers
        while (BOOST_LWCO_INTERLOCKED_CMP_XCHG(&trigger, BOOST_LWCO_INIT, BOOST_LWCO_INIT) != 2)
        {
            if (BOOST_LWCO_INTERLOCKED_CMP_XCHG(&trigger, 1, BOOST_LWCO_INIT) == BOOST_LWCO_INIT)
            {
                // Only one thread passes here
                try
                {
                    fun();
                    BOOST_LWCO_INTERLOCKED_INC(&trigger);
                    break;
                }
                catch (...)
                {
                    BOOST_LWCO_INTERLOCKED_XCHG(&trigger, BOOST_LWCO_INIT);
                    throw;
                }
            }
            else
            {
                // Other threads wait here until the function object execution is complete
                while (BOOST_LWCO_INTERLOCKED_CMP_XCHG(&trigger, 1, 1) == 1)
                {
                    // It has to be non-zero since a lower-priority thread could have entered the functor
                    Sleep(1);
                }
            }
        }
    }

#else // BOOST_LWCO_NO_NT6_WINAPI_SUPPORT

    //! The type used to trigger the execution
    struct call_once_trigger
    {
        volatile long done;
        SRWLOCK lock;
    };

    //! A simple auto-lock for SRW
    struct scoped_call_once_lock
    {
        explicit scoped_call_once_lock(call_once_trigger& trigger) : m_lock(&trigger.lock)
        {
            AcquireSRWLockExclusive(m_lock);
        }
        ~scoped_call_once_lock()
        {
            ReleaseSRWLockExclusive(m_lock);
        }

    private:
        //! A pointer to the SRW lock
        SRWLOCK* m_lock;
    };

    //! Initial trigger value
#   ifdef BOOST_USE_WINDOWS_H
#       define BOOST_LWCO_INIT { 0, SRWLOCK_INIT }
#   else
#       define BOOST_LWCO_INIT { 0, { 0 } }
#   endif // BOOST_USE_WINDOWS_H

    //! The function calls functor fun only if the trigger is in init state
    template< typename T >
    inline void call_once(call_once_trigger& trigger, T const& fun)
    {
#   ifndef BOOST_LWCO_NO_MEMORY_BARRIERS
        BOOST_READ_MEMORY_BARRIER();
        if (!trigger.done)
#   else
        if (BOOST_LWCO_INTERLOCKED_CMP_XCHG(&trigger.done, 0, 0) == 0)
#   endif // !BOOST_LWCO_NO_MEMORY_BARRIERS
        {
            scoped_call_once_lock lock(trigger);
            if (!trigger.done)
            {
                fun();
                trigger.done = 1;
            }
        }
    }

#endif // BOOST_LWCO_NO_NT6_WINAPI_SUPPORT

} // namespace lw_call_once

} // namespace detail

} // namespace boost

#undef BOOST_LWCO_INTERLOCKED_CMP_XCHG
#undef BOOST_LWCO_INTERLOCKED_INC
#undef BOOST_LWCO_NO_NT6_WINAPI_SUPPORT
#undef BOOST_LWCO_NO_MEMORY_BARRIERS

#endif // BOOST_LIGHTWEIGHT_CALL_ONCE_WIN32_HPP_INCLUDED_
