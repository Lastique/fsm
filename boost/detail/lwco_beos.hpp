/*!
 * (C) 2006 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   lwco_beos.hpp
 * \author Andrey Semashev
 * \date   10.02.2007
 * 
 * \brief  This header is the call_once concept light implementation for BeOS threading API.
 */

#ifndef BOOST_LIGHTWEIGHT_CALL_ONCE_BEOS_HPP_INCLUDED_
#define BOOST_LIGHTWEIGHT_CALL_ONCE_BEOS_HPP_INCLUDED_

#include <be/kernel/OS.h>
#include <be/support/SupportDefs.h>

#if !defined(BOOST_LWCO_NO_EXPLICIT_MEMORY_BARRIERS)
#   include <boost/detail/memory_barrier.hpp>
#   ifdef BOOST_NO_MEMORY_BARRIERS
#       define BOOST_LWCO_USE_SEMAPHORES 1
#   endif // BOOST_NO_MEMORY_BARRIERS
#endif // !defined(BOOST_LWCO_NO_EXPLICIT_MEMORY_BARRIERS)

#if defined(BOOST_LWCO_USE_SEMAPHORES)
#include <cstdio>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <boost/throw_exception.hpp>
#endif // defined(BOOST_LWCO_USE_SEMAPHORES)

namespace boost {

namespace detail {

namespace lw_call_once {

#if !defined(BOOST_LWCO_USE_SEMAPHORES)

    //! The type used to trigger the execution
    typedef vint32 call_once_trigger;
    //! Initial trigger value
    #define BOOST_LWCO_INIT 0

    //! The function calls functor fun only if the trigger is in init state
    template< typename T >
    inline void call_once(call_once_trigger& trigger, T const& fun)
    {
        BOOST_READ_MEMORY_BARRIER();
        while (atomic_and(&trigger, 3) != 3)
        {
            if (atomic_or(&trigger, 1) == 0)
            {
                // Only single thread is passed here
                try
                {
                    fun();
                    BOOST_WRITE_MEMORY_BARRIER();
                    atomic_or(&trigger, 2);
                    break;
                }
                catch (...)
                {
                    atomic_and(&trigger, 0);
                    throw;
                }
            }
            else
            {
                // All other threads sleep each 500 microseconds until fun execution is compete
                while (atomic_and(&trigger, 3) == 1)
                    snooze(500);
            }
        }
    }

#else // !defined(BOOST_LWCO_USE_SEMAPHORES)

    //  For other compilers we have no memory barriers, which forces us to a slower reference implementation

    //! The type used to trigger the execution
    struct call_once_trigger
    {
        //! The flag shows whether the function object was called
        volatile bool flag;
        //! The thread counter used to synchronize semaphore creation
        vint32 lock;
        //! The semaphore that synchronize function object call and acts as a memory barrier
        sem_id semaphore;
    };
    //! Initial trigger value
    #define BOOST_LWCO_INIT { false, 0, 0 }

    //! A simple semaphore auto locker/unlocker
    class auto_locker
    {
        //! Semaphore identifier
        sem_id m_semaphore;

    public:
        //! Constructor - acquires the semaphore
        explicit auto_locker(sem_id semaphore) : m_semaphore(semaphore)
        {
            status_t status = acquire_sem(semaphore);
            if (status != B_NO_ERROR)
            {
                const std::size_t message_len = sizeof("lightweight call_once: failed to acquire semaphore, error code: ") - 1;
                char message[message_len + std::numeric_limits< int >::digits10 + 1];
                std::sprintf(message, "lightweight call_once: failed to acquire semaphore, error code: %d", (int)status);
                boost::throw_exception(std::runtime_error(message));
            }
        }
        //! Destructor - releases the semaphore
        ~auto_locker()
        {
            release_sem_etc(m_semaphore, 1, B_DO_NOT_RESCHEDULE);
        }
    };

    //! A guard object used to destroy the semaphore on module unload
    class auto_sem_deleter
    {
        //! Semaphore identifier
        sem_id m_semaphore;

    public:
        explicit auto_sem_deleter(sem_id semaphore) : m_semaphore(semaphore)
        {
        }
        ~auto_sem_deleter()
        {
            delete_sem(m_semaphore);
        }
    };

    //! The function calls functor fun only if the trigger is in init state
    template< typename T >
    inline void call_once(call_once_trigger& trigger, T const& fun)
    {
        sem_id sem = atomic_or((vint32*)&trigger.semaphore, 0);
        if (sem == 0)
        {
            // Synchronize semaphore creation - lock
            int32 was_locked = atomic_add(&trigger.lock, 1);
            if (was_locked != 0)
            {
                // Wait until the semaphore is created
                atomic_add(&trigger.lock, -1);
                while (atomic_or(&trigger.lock, 0) != 0)
                    snooze(500);
            }
            // Create semaphore, if needed
            sem = atomic_or((vint32*)&trigger.semaphore, 0);
            if (sem == 0)
            {
                sem = create_sem(1, "boost lightweight call_once semaphore");
                if (sem < B_NO_ERROR)
                {
                    atomic_add(&trigger.lock, -1);

                    const std::size_t message_len = sizeof("lightweight call_once: failed to create semaphore, error code: ") - 1;
                    char message[message_len + std::numeric_limits< int >::digits10 + 1];
                    std::sprintf(message, "lightweight call_once: failed to create semaphore, error code: %d", (int)sem);
                    boost::throw_exception(std::runtime_error(message));
                }
                atomic_or((vint32*)&trigger.semaphore, (int32)sem);
                // This will automatically destroy the semaphore
                static volatile auto_sem_deleter sem_deleter(sem);
            }
            // Synchronize semaphore creation - unlock
            atomic_add(&trigger.lock, -1);
        }

        // Acquire semaphore - should act as a MB
        auto_locker lock(sem);

        if (!trigger.flag)
        {
            // Only single thread is passed here
            fun();
            trigger.flag = true;
        }
    }

#endif // !defined(BOOST_LWCO_USE_SEMAPHORES)

} // namespace lw_call_once

} // namespace detail

} // namespace boost

#undef BOOST_LWCO_USE_SEMAPHORES

#endif // BOOST_LIGHTWEIGHT_CALL_ONCE_BEOS_HPP_INCLUDED_
