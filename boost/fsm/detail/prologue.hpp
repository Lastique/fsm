/*!
 * (C) 2006 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   prologue.hpp
 * \author Andrey Semashev
 * \date   23.12.2006
 * 
 * \brief  This header is the Boost.FSM library implementation, see the library documentation
 *         at http://www.boost.org/libs/state_machine/doc/state_machine.html. In this file
 *         an internal configuration macros are defined.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_FSM_DETAIL_PROLOGUE_HPP_INCLUDED_
#define BOOST_FSM_DETAIL_PROLOGUE_HPP_INCLUDED_

#include <boost/config.hpp>

// Extended declaration macros. Used to implement compiler-specific optimizations.
#if defined(_MSC_VER)
#    define BOOST_FSM_FASTCALL __fastcall
#    define BOOST_FSM_NOINLINE __declspec(noinline)
#    define BOOST_FSM_FORCEINLINE __forceinline
#    define BOOST_FSM_NO_VTABLE __declspec(novtable)
#elif defined(__GNUC__)
#    define BOOST_FSM_NOINLINE __attribute__((noinline))
#    if (__GNUC__ > 3)
#        define BOOST_FSM_FORCEINLINE inline __attribute__((always_inline))
#        if defined(linux) || defined(__linux) || defined(__linux__)
#            define BOOST_FSM_EXTERNALLY_VISIBLE __attribute__((visibility("default")))
#        endif // defined(linux) || defined(__linux) || defined(__linux__)
#    else
#        define BOOST_FSM_FORCEINLINE inline
#    endif
#    if (((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)) || (__GNUC__ > 3))
#        define BOOST_FSM_FASTCALL __attribute__((fastcall))
#    else
#        define BOOST_FSM_FASTCALL __attribute__((regparm(2)))
#    endif
#    define BOOST_FSM_NO_VTABLE
#else
#    define BOOST_FSM_FASTCALL
#    define BOOST_FSM_NOINLINE
#    define BOOST_FSM_FORCEINLINE inline
#    define BOOST_FSM_NO_VTABLE
#endif

#ifndef BOOST_FSM_EXTERNALLY_VISIBLE
#    define BOOST_FSM_EXTERNALLY_VISIBLE
#endif

// An MS-like compilers' extension that allows to optimize away the needless code
#if defined(_MSC_VER)
#define BOOST_FSM_ASSUME(expr) __assume(expr)
#else
#define BOOST_FSM_ASSUME(expr)
#endif

#endif // BOOST_FSM_DETAIL_PROLOGUE_HPP_INCLUDED_
