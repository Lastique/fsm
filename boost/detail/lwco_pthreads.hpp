/*!
 * (C) 2006 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   lwco_pthreads.hpp
 * \author Andrey Semashev
 * \date   07.02.2007
 * 
 * \brief  This header is the call_once concept light implementation for POSIX threading API.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1020)
#pragma once
#endif // _MSC_VER > 1020

#ifndef BOOST_LIGHTWEIGHT_CALL_ONCE_PTHREADS_HPP_INCLUDED_
#define BOOST_LIGHTWEIGHT_CALL_ONCE_PTHREADS_HPP_INCLUDED_

#include <cstdio>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <pthread.h>
#include <boost/throw_exception.hpp>

#ifndef BOOST_LWCO_NO_EXPLICIT_MEMORY_BARRIERS

#   include <boost/detail/memory_barrier.hpp>

#   ifndef BOOST_NO_MEMORY_BARRIERS

// We use atomics only if there is support for explicit memory barriers
#      if defined(__GLIBCPP__) || defined(__GLIBCXX__)
#           define BOOST_LWCO_HAS_GNU_ATOMIC 1
#      elif (defined(sun) || defined(__sun)) && (defined(__SunOS_5_10) || defined(__SunOS_5_11))
#           define BOOST_LWCO_HAS_SOLARIS_ATOMIC 1
#      else
#           define BOOST_LWCO_NO_ATOMICS 1
#      endif

#   else // BOOST_NO_MEMORY_BARRIERS

#       define BOOST_LWCO_NO_ATOMICS 1

#   endif // BOOST_NO_MEMORY_BARRIERS

#else // !BOOST_LWCO_NO_EXPLICIT_MEMORY_BARRIERS

#   define BOOST_LWCO_NO_ATOMICS 1

#endif // !BOOST_LWCO_NO_EXPLICIT_MEMORY_BARRIERS

#ifndef BOOST_LWCO_NO_ATOMICS
#   include <unistd.h>
#endif // !BOOST_LWCO_NO_ATOMICS

#if defined(BOOST_LWCO_HAS_GNU_ATOMIC)
#   if defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 402
#       include <ext/atomicity.h>
#   else
#       include <bits/atomicity.h>
#   endif // defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 402
#elif defined(BOOST_LWCO_HAS_SOLARIS_ATOMIC)
#   include <atomic.h>
#endif

namespace boost {

namespace detail {

namespace lw_call_once {

#if !defined(BOOST_LWCO_NO_ATOMICS)

#if defined(BOOST_LWCO_HAS_GNU_ATOMIC)

#if defined(__GLIBCXX__) // g++ 3.4+
    using __gnu_cxx::__atomic_add;
    using __gnu_cxx::__exchange_and_add;
#endif // defined(__GLIBCXX__)

    //! The type used to trigger the execution
    typedef volatile _Atomic_word atomic_type;
    //! Initial trigger value
#   define BOOST_LWCO_ATOMIC_TYPE_INIT 0

#   define BOOST_LWCO_ATOMIC_IS_EQUAL(x, y) (__exchange_and_add(&(x), 0) == (y))
#   define BOOST_LWCO_ATOMIC_INCREMENT(x) __atomic_add(&(x), 1)
#   define BOOST_LWCO_ATOMIC_DECREMENT(x) __atomic_add(&(x), -1)

#elif defined(BOOST_LWCO_HAS_SOLARIS_ATOMIC)

    //! The type used to trigger the execution
    typedef uint32_t atomic_type;
    //! Initial trigger value
#   define BOOST_LWCO_ATOMIC_TYPE_INIT 0

#   define BOOST_LWCO_ATOMIC_IS_EQUAL(x, y) (atomic_or_32(&(x), 0) == (y))
#   define BOOST_LWCO_ATOMIC_INCREMENT(x) atomic_add_32(&(x), 1)
#   define BOOST_LWCO_ATOMIC_DECREMENT(x) atomic_add_32(&(x), -1)

#endif

    //! The type used to trigger the execution
    struct call_once_trigger
    {
        //! The flag that shows whether the once routine have already been called
        volatile bool done;
        //! Thread counter to destroy the mutex safely
        atomic_type thread_count;
        //! Synchronization mutex
        pthread_mutex_t mutex;
    };
    //! Initial trigger value
#   define BOOST_LWCO_INIT { false, BOOST_LWCO_ATOMIC_TYPE_INIT, PTHREAD_MUTEX_INITIALIZER }

    //! A simple auto-lock for the pthread mutex
    struct scoped_call_once_lock
    {
        explicit scoped_call_once_lock(call_once_trigger& trigger) : m_trigger(trigger)
        {
            register int result = pthread_mutex_lock(&m_trigger.mutex);
            if (result != 0)
            {
                const std::size_t message_len = sizeof("lightweight call_once: pthread_mutex_lock failed, error code: ") - 1;
                char message[message_len + std::numeric_limits< int >::digits10 + 1];
                std::sprintf(message, "lightweight call_once: pthread_mutex_lock failed, error code: %d", (int)result);
                boost::throw_exception(std::runtime_error(message));
            }
        }
        ~scoped_call_once_lock()
        {
            pthread_mutex_unlock(&m_trigger.mutex);
            BOOST_LWCO_ATOMIC_DECREMENT(m_trigger.thread_count);
        }

    private:
        //! A reference to the trigger object
        call_once_trigger& m_trigger;
    };

    //! The function calls functor fun only if the trigger is in init state
    template< typename T >
    inline void call_once(call_once_trigger& trigger, T const& fun)
    {
        BOOST_READ_MEMORY_BARRIER();
        if (!trigger.done)
        {
            BOOST_LWCO_ATOMIC_INCREMENT(trigger.thread_count);
            if (!trigger.done)
            {
                bool destroy_mutex = false;
                {
                    // The lock decrements trigger.thread_count on destruction
                    // It will also act as a WMB
                    scoped_call_once_lock lock(trigger);
                    if (!trigger.done)
                    {
                        fun();
                        destroy_mutex = true;
                        trigger.done = true;
                    }
                }

                if (destroy_mutex)
                {
                    // Wait here until other threads release the mutex
                    while (!BOOST_LWCO_ATOMIC_IS_EQUAL(trigger.thread_count, 0))
                        usleep(500);
                    pthread_mutex_destroy(&trigger.mutex);
                }
            }
            else
            {
                BOOST_LWCO_ATOMIC_DECREMENT(trigger.thread_count);
            }
        }
    }

#undef BOOST_LWCO_ATOMIC_IS_EQUAL
#undef BOOST_LWCO_ATOMIC_INCREMENT
#undef BOOST_LWCO_ATOMIC_DECREMENT

#else // !defined(BOOST_LWCO_NO_ATOMICS)

    // For other compilers/platforms there's less efficient code

    //! The type used to trigger the execution
    struct call_once_trigger
    {
        //! The flag that shows whether the once routine have already been called
        volatile bool done;
        //! The synchronization lock
        pthread_rwlock_t lock;
    };
    //! Initial trigger value
#   define BOOST_LWCO_INIT { false, PTHREAD_RWLOCK_INITIALIZER }

    //! A simple auto locker
    struct mutex_auto_locker
    {
        explicit mutex_auto_locker(pthread_rwlock_t* lock) : m_lock(lock)
        {
            register int result = pthread_rwlock_wrlock(lock);
            if (result != 0)
            {
                const std::size_t message_len = sizeof("lightweight call_once: pthread_rwlock_wrlock failed, error code: ") - 1;
                char message[message_len + std::numeric_limits< int >::digits10 + 1];
                std::sprintf(message, "lightweight call_once: pthread_rwlock_wrlock failed, error code: %d", (int)result);
                boost::throw_exception(std::runtime_error(message));
            }
        }
        ~mutex_auto_locker()
        {
            pthread_rwlock_unlock(m_lock);
        }

    private:
        //! A pointer to the mutex
        pthread_rwlock_t* m_lock;
    };

    //! A guard class to automatically destroy the mutex when the module gets unloaded
    class mutex_auto_destroyer
    {
    private:
        //! A pointer to the mutex
        pthread_rwlock_t* m_lock;

    public:
        explicit mutex_auto_destroyer(pthread_rwlock_t* lock) : m_lock(lock)
        {
        }
        ~mutex_auto_destroyer()
        {
            // This should be quite safe since the destroyer object
            // destructs on module unloading, when no other threads
            // should be running this module code
            pthread_rwlock_destroy(m_lock);
        }
    };

    //! The function calls functor fun only if the trigger is in init state
    template< typename T >
    inline void call_once(call_once_trigger& trigger, T const& fun)
    {
        // This read lock should act as a RMB
        pthread_rwlock_rdlock(&trigger.lock);
        register bool done = trigger.done;
        pthread_rwlock_unlock(&trigger.lock);

        if (!done)
        {
            // This lock should act as a WMB
            mutex_auto_locker lock(&trigger.lock);
            if (!trigger.done)
            {
                // Let's hope that this object's destructor will finally be called...
                static volatile mutex_auto_destroyer destroyer(&trigger.lock);
                fun();
                trigger.done = true;
            }
        }
    }

#endif // !defined(BOOST_LWCO_NO_ATOMICS)

} // namespace lw_call_once

} // namespace detail

} // namespace boost

#undef BOOST_LWCO_NO_ATOMICS
#undef BOOST_LWCO_HAS_GNU_ATOMIC
#undef BOOST_LWCO_HAS_SOLARIS_ATOMIC

#endif // BOOST_LIGHTWEIGHT_CALL_ONCE_PTHREADS_HPP_INCLUDED_
