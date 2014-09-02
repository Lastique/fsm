/*!
 * (C) 2006 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   locking_state_machine.hpp
 * \author Andrey Semashev
 * \date   24.11.2006
 * 
 * \brief  This header is the Boost.FSM library implementation, see the library documentation
 *         at http://www.boost.org/libs/state_machine/doc/state_machine.html
 */

#if (defined(_MSC_VER) && _MSC_VER > 1020)
#pragma once
#endif // _MSC_VER > 1020

#ifndef BOOST_FSM_LOCKING_STATE_MACHINE_HPP_INCLUDED_
#define BOOST_FSM_LOCKING_STATE_MACHINE_HPP_INCLUDED_

#include <new>
#include <functional>
#include <boost/noncopyable.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/detail/lightweight_mutex.hpp>
#include <boost/fsm/state_machine.hpp>

namespace boost {

namespace fsm {

namespace aux {

    //! A simple scope locker class to dynamically decide when to lock the mutex
    template< typename LockerT >
    class dynamic_locker
    {
        //! A storage for the locker
        aligned_storage< sizeof(LockerT) > m_Storage;
        //! A flag designating that the locker has actually been constructed
        volatile bool m_fLocked;

    public:
        //! Constructor
        dynamic_locker() : m_fLocked(false)
        {
        }
        //! Destructor
        ~dynamic_locker()
        {
            if (m_fLocked)
            {
                LockerT* const p = reinterpret_cast< LockerT* >(m_Storage.address());
                p->~LockerT();
            }
        }

        //! The method locks the mutex
        template< typename MutexT >
        void lock(MutexT& mtx)
        {
            // We construct a locker here and writing to a volatile variable.
            // It is said (1.9/8) that functions do not interleave and (1.9/5, 1.9/6)
            // volatile writes are strictly sequential. This makes sure the compiler
            // won't shuffle the lock method calls and thus lock mutexes in the right order.
            new (m_Storage.address()) LockerT(mtx);
            m_fLocked = true;
        }
    };

} // namespace aux

//! An implementation of thread-locking state machine
template<
    typename StateListT,
    typename RetValT = void,
    typename TransitionListT = void,
    typename MutexT = detail::lightweight_mutex,
    typename LockerT = typename MutexT::scoped_lock
>
class locking_state_machine :
    public aux::basic_state_machine< StateListT, RetValT, TransitionListT >
{
private:
    //! Base type
    typedef aux::basic_state_machine< StateListT, RetValT, TransitionListT > base_type;

protected:
    //! Root type
    typedef typename base_type::root_type root_type;

public:
    //! Return type import
    typedef typename base_type::return_type return_type;

    //! Mutex type
    typedef MutexT mutex_type;
    //! Locker type
    typedef LockerT scoped_lock;

private:
    //! Mutex to lock the object
    mutable mutex_type m_Mutex;

public:
    /*!
    *    \brief Default constructor
    *    \throw Nothing unless a state constructor throws
    */
    locking_state_machine()
    {
    }
    /*!
    *    \brief Copying constructor
    *    \throw Nothing unless a state constructor throws
    */
    locking_state_machine(locking_state_machine const& that)
        // We use here comma operator trick to lock the argument right before the copying begins
        : base_type((scoped_lock(that.get_mutex()), static_cast< base_type const& >(that))), m_Mutex()
    {
    }
    /*!
    *    \brief Copying constructor from locking_state_machine with other threading strategies
    *    \throw Nothing unless a state constructor throws
    */
    template< typename AnotherMutexT, typename AnotherLockerT >
    locking_state_machine(locking_state_machine<
        StateListT,
        RetValT,
        TransitionListT,
        AnotherMutexT,
        AnotherLockerT
    > const& that)
        // We use here comma operator trick to lock the argument right before the copying begins
        : base_type((typename locking_state_machine<
            StateListT,
            RetValT,
            TransitionListT,
            AnotherMutexT,
            AnotherLockerT
        >::scoped_lock(that.get_mutex()), static_cast< base_type const& >(that))), m_Mutex()
    {
    }
    /*!
    *    \brief A constructor with automatic unexpected events handler setting
    *    \param handler Unexpected event handler, see set_unexpected_event_handler comment.
    *    \throw Nothing unless a state constructor or set_unexpected_event_handler throws
    */
    template< typename T >
    locking_state_machine(T const& handler) : base_type(handler)
    {
    }

    /*!
    *    \brief Assignment operator
    *    \throw Nothing unless a state assignment or locker throws
    */
    locking_state_machine& operator= (locking_state_machine const& that)
    {
        // Thanks to Steven Watanabe for this idea
        // of safe-locking two mutexes to avoid deadlock
        const locking_state_machine* const p = addressof(that);
        if (this != p)
        {
            // According to 5.9/2 and 20.3.3/8 we use std::less to guarantee the ordering of the pointers
            std::less< const locking_state_machine* > ordering;
            const bool lock_order = ordering(this, p);
            scoped_lock first_lock(lock_order ? get_mutex() : that.get_mutex());
            scoped_lock second_lock(lock_order ? that.get_mutex() : get_mutex());
            base_type::operator= (static_cast< base_type const& >(that));
        }
        return *this;
    }

    /*!
    *    \brief Assignment operator from locking_state_machine with other threading strategies
    *    \throw Nothing unless a state assignment or locker throws
    */
    template< typename AnotherMutexT, typename AnotherLockerT >
    locking_state_machine& operator= (locking_state_machine<
        StateListT,
        RetValT,
        TransitionListT,
        AnotherMutexT,
        AnotherLockerT
    > const& that)
    {
        // Define types of that state machine and its locker
        typedef locking_state_machine<
            StateListT,
            RetValT,
            TransitionListT,
            AnotherMutexT,
            AnotherLockerT
        > that_type;
        typedef typename that_type::scoped_lock that_scoped_lock;

        // We use here aligned_storage to create lockers in a specific order
        aux::dynamic_locker< scoped_lock > this_lock;
        aux::dynamic_locker< that_scoped_lock > that_lock;

        // According to 5.9/2 and 20.3.3/8 we use std::less to guarantee the ordering of the pointers
        std::less< const void* > ordering;
        if (ordering(reinterpret_cast< const void* >(this), reinterpret_cast< const void* >(addressof(that))))
        {
            this_lock.lock(get_mutex());
            that_lock.lock(that.get_mutex());
        }
        else
        {
            that_lock.lock(that.get_mutex());
            this_lock.lock(get_mutex());
        }
        base_type::operator= (static_cast< base_type const& >(that));

        return *this;
    }

    /*!
    *    \brief Event processing routine
    *    \param evt The event to pass to state machine
    *    \return The result of on_process handler called or, in case if no handler found, the result of an unexpected event routine
    *    \throw Nothing unless the state_machine::process or locker throws
    */
    template< typename EventT >
    return_type process(EventT const& evt)
    {
        scoped_lock lock(m_Mutex);
        return base_type::process(evt);
    }

    /*!
    *    \brief The method resets the state machine to its initial state
    *    \throw Nothing unless locker throws
    */
    void reset()
    {
        scoped_lock lock(m_Mutex);
        base_type::reset();
    }

    /*!
    *    \brief The method sets an unexpected events handler
    *    \sa state_machine_root::set_unexpected_event_handler
    */
    template< typename T >
    void set_unexpected_event_handler(T const& handler)
    {
        scoped_lock lock(m_Mutex);
        base_type::set_unexpected_event_handler(handler);
    }
    /*!
    *    \brief The method resets the unexpected events handler to the default
    *    \sa state_machine_root::set_default_unexpected_event_handler
    */
    void set_default_unexpected_event_handler()
    {
        scoped_lock lock(m_Mutex);
        base_type::set_default_unexpected_event_handler();
    }

    /*!
    *    \brief An accessor to the state machine mutex
    *    \throw None
    */
    mutex_type& get_mutex() const { return m_Mutex; }
};

} // namespace fsm

} // namespace boost

#endif // BOOST_FSM_LOCKING_STATE_MACHINE_HPP_INCLUDED_
