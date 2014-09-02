/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   memory_barrier.hpp
 * \author Andrey Semashev
 * \date   09.02.2007
 * 
 * \brief  This header is for memory barrier compatibility.
 *
 * The header defines macros BOOST_READ_MEMORY_BARRIER, BOOST_WRITE_MEMORY_BARRIER and
 * BOOST_READ_WRITE_MEMORY_BARRIER if the appropriate support from the compiler and platform
 * is detected. If no such support is found, then BOOST_NO_MEMORY_BARRIERS is defined.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_MEMORY_BARRIER_HPP_INCLUDED_
#define BOOST_MEMORY_BARRIER_HPP_INCLUDED_

#include <boost/config.hpp>

#if (defined(BOOST_MSVC) || defined(BOOST_INTEL_WIN)) && !defined(_M_CEE_PURE) && (defined(_M_IX86) || defined(_M_X64))

#    if _MSC_VER < 1310

// Not sure if VC 7.0 has MB support, but VC 6 doesn't
#        define BOOST_NO_MEMORY_BARRIERS 1

#    else // _MSC_VER < 1310

#        include <intrin.h>

#        pragma intrinsic(_ReadWriteBarrier)
#        define BOOST_READ_WRITE_MEMORY_BARRIER() _ReadWriteBarrier()

#        if _MSC_VER >= 1400
#            pragma intrinsic(_ReadBarrier)
#            pragma intrinsic(_WriteBarrier)
#            define BOOST_READ_MEMORY_BARRIER() _ReadBarrier()
#            define BOOST_WRITE_MEMORY_BARRIER() _WriteBarrier()
#        else // _MSC_VER >= 1400
#            define BOOST_READ_MEMORY_BARRIER() _ReadWriteBarrier()
#            define BOOST_WRITE_MEMORY_BARRIER() _ReadWriteBarrier()
#        endif // _MSC_VER >= 1400

#    endif // _MSC_VER < 1310

#elif defined(__GLIBCPP__) || defined(__GLIBCXX__)

#    if defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 402
#        include <ext/atomicity.h>
#    else
#        include <bits/atomicity.h>
#    endif // defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 402

#    if !defined(_GLIBCXX_READ_MEM_BARRIER) || !defined(_GLIBCXX_WRITE_MEM_BARRIER)
#        define BOOST_NO_MEMORY_BARRIERS 1
#    else // !defined(_GLIBCXX_READ_MEM_BARRIER) || !defined(_GLIBCXX_WRITE_MEM_BARRIER)
#        define BOOST_READ_MEMORY_BARRIER() _GLIBCXX_READ_MEM_BARRIER
#        define BOOST_WRITE_MEMORY_BARRIER() _GLIBCXX_WRITE_MEM_BARRIER
#        define BOOST_READ_WRITE_MEMORY_BARRIER() BOOST_READ_MEMORY_BARRIER(); BOOST_WRITE_MEMORY_BARRIER()
#    endif // !defined(_GLIBCXX_READ_MEM_BARRIER) || !defined(_GLIBCXX_WRITE_MEM_BARRIER)

#elif (defined(sun) || defined(__sun)) && (defined(__SunOS_5_10) || defined(__SunOS_5_11))

// The condition above will need updating once new versions of SunOS are out
#    include <atomic.h>

#    define BOOST_READ_MEMORY_BARRIER() membar_consumer()
#    define BOOST_WRITE_MEMORY_BARRIER() membar_producer()
#    define BOOST_READ_WRITE_MEMORY_BARRIER() membar_exit()

#else

#    define BOOST_NO_MEMORY_BARRIERS 1

#endif

#endif // BOOST_MEMORY_BARRIER_HPP_INCLUDED_
