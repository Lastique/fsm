/*!
* (C) 2007 Andrey Semashev
* 
* Use, modification and distribution is subject to the Boost Software License, Version 1.0.
* (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*
* \file   bit_machine.cpp
* \author Andrey Semashev
* \date   20.01.2007
* 
* \brief  A BitMachine sample code to compare Boost.FSM implementation to Boost.Statechart
*
* See BitMachine example code of Boost.Statechart library for the description of the test.
* You may configure the test with these macros:
* - NO_OF_BITS. The number of bits, it actually controls the size of the generated FSM.
*   The state machine will contain 2 ^ NO_OF_BITS states and will be processing
*   up to NO_OF_BITS event types.
* - FORCE_MULTIPLE_TRANSITIONS. If this macro is not defined there will be a single
*   template transition that switches states on receiving events. This may reduce
*   compilation time but is different from the Boost.Statechart example implementation,
*   where each state has its own list of transitions to other states. When this macro
*   is defined the generated FSM will have a global list of NO_OF_BITS transitions,
*   which is equivalent to having the same list in each state separatedly (which would
*   yeld 2 ^ NO_OF_BITS * NO_OF_BITS transitions in total that we have in Boost.Statechart).
* - NO_TRANSITION_MAPS. Disables transition maps usage, all transitions are made from event handlers.
* - NO_OF_PERFORMANCE_EVENTS. The number of events to pass to the FSM during the performance test.
*/

#include <ctime>
#include <iomanip>
#include <iostream>
#include <boost/config.hpp>
#include <boost/bind.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/advance.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/fsm/state_machine.hpp>
#include <boost/fsm/event.hpp>
#include <boost/fsm/transition.hpp>
#if defined(BOOST_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !defined(WIN32_LEAN_AND_MEAN)
#include "windows.h"
#endif // defined(BOOST_WINDOWS)

//  Number of bits to generate FSM for.
//  This constant severilly influences the compilation time and required resources.
//  For example, when equal 6 the compilation on VC8 may take a minute requiring about 800 Mb RAM.
//  GCC 4.1.1 takes about 400 Mb RAM and Intel C++ Comiler for Windows - about 1 Gb RAM for the same value.
#ifndef NO_OF_BITS
#define NO_OF_BITS 3
#endif // NO_OF_BITS

#ifndef NO_OF_PERFORMANCE_EVENTS
#define NO_OF_PERFORMANCE_EVENTS 100000000UL
#endif // NO_OF_PERFORMANCE_EVENTS

//////////////////////////////////////////////////////////////////////////
//  Bit state machine implementation
//////////////////////////////////////////////////////////////////////////

//! State class forward
template< unsigned int >
struct BitState;

//! The number of states
enum { NO_OF_STATES = (1 << NO_OF_BITS) };

//! A structure to generate states list
struct StatesList
{
    struct tag {};

    template< unsigned int N >
    struct iterator
    {
        typedef BitState< N > type;
        typedef iterator< N + 1 > next;
    };
    typedef iterator< 0 > begin;
    typedef iterator< NO_OF_STATES > end;
};

//  Additional MPL specializations to make StatesList operations more efficient
namespace boost { namespace mpl {

template< unsigned int N1, unsigned int N2 >
struct distance< StatesList::iterator< N1 >, StatesList::iterator< N2 > >
{
    typedef long_< N2 - N1 > type;
    enum { value = type::value };
};

template< unsigned int N, long ShiftV >
struct advance_c< StatesList::iterator< N >, ShiftV >
{
    typedef StatesList::iterator< N + ShiftV > type;
};
template< unsigned int N, typename ShiftT >
struct advance< StatesList::iterator< N >, ShiftT >
{
    typedef StatesList::iterator< N + ShiftT::value > type;
};

} } // namespace boost::mpl

//! State implementation
template< unsigned int ValueV >
struct BitState :
    public boost::fsm::state< BitState< ValueV >, StatesList >
{
    //! A generic processor of any events
    template< int BitNoV >
    void on_process(boost::fsm::event_c< BitNoV > const&)
    {
#ifdef NO_TRANSITION_MAPS
        enum { NextValue = ValueV ^ (1 << BitNoV) };
        typedef BitState< NextValue > NextState_t;
        this->BOOST_NESTED_TEMPLATE switch_to< NextState_t >();
#endif // NO_TRANSITION_MAPS
    }
};


#ifndef NO_TRANSITION_MAPS

#ifndef FORCE_MULTIPLE_TRANSITIONS

//! Transition implementation
struct BitTransition
{
    template< typename StateT, typename EventT >
    struct is_applicable : boost::mpl::true_ {};

    //! The transition flips the BitNoV'th bit in the ValueV and switches to the result state
    template< unsigned int ValueV, int BitNoV >
    static void transit(BitState< ValueV >& state, boost::fsm::event_c< BitNoV > const&)
    {
        enum { NextValue = ValueV ^ (1 << BitNoV) };
        typedef BitState< NextValue > NextState_t;
        state.BOOST_NESTED_TEMPLATE switch_to< NextState_t >();
    }
};

//! Transitions list
typedef boost::mpl::vector< BitTransition >::type TransitionsList_t;

#else // !defined(FORCE_MULTIPLE_TRANSITIONS)

template< int BitNoV >
struct BitTransition
{
    template< typename StateT, typename EventT >
    struct is_applicable :
        boost::is_same< EventT, boost::fsm::event_c< BitNoV > >
    {
    };

    template< unsigned int ValueV >
    static void transit(BitState< ValueV >& state, boost::fsm::event_c< BitNoV > const&)
    {
        enum { NextValue = ValueV ^ (1 << BitNoV) };
        typedef BitState< NextValue > NextState_t;
        state.BOOST_NESTED_TEMPLATE switch_to< NextState_t >();
    }
};

#define MAKE_TRANSITION_TYPE(z, iter, data) BitTransition< iter >

//! Transitions list
typedef boost::mpl::vector< BOOST_PP_ENUM(NO_OF_BITS, MAKE_TRANSITION_TYPE, ~) >::type TransitionsList_t;

#endif // !defined(FORCE_MULTIPLE_TRANSITIONS)

#else // !defined(NO_TRANSITION_MAPS)

typedef void TransitionsList_t;

#endif // !defined(NO_TRANSITION_MAPS)

//! State machine type
typedef boost::fsm::state_machine< StatesList, void, TransitionsList_t > BitFSM_t;


//////////////////////////////////////////////////////////////////////////
//  Test implementation
//////////////////////////////////////////////////////////////////////////

//! The function prints the current FSM state name
inline void print_current_state_name(BitFSM_t const& fsm)
{
    std::cout << "The current state is: " << fsm.get_current_state_name() << std::endl;
}

// The type of trampolines
typedef void (*trampoline_t)(BitFSM_t& fsm);

//! A function object to pass an event to the state machine
struct invoke_sm
{
    //! Return type
    typedef void result_type;

private:
    //! A bound reference to the machine
    BitFSM_t& m_fsm;

public:
    //! Constructor
    invoke_sm(BitFSM_t& fsm) : m_fsm(fsm) {}

    //! An operator that invokes the machine
    template< typename BitNoT >
    void operator() (BitNoT const&) const
    {
        m_fsm.process(boost::fsm::make_event< BitNoT::value >());
    }

    //! The trampoline function to pass an event to the machine
    template< int BitNoV >
    static void trampoline(BitFSM_t& fsm)
    {
        invoke_sm invoker(fsm);
        invoker(boost::mpl::int_< BitNoV >());
    }
};

//! Performance test implementation
void run_performance_test(BitFSM_t& fsm)
{
    invoke_sm invoker(fsm);
    const unsigned long end = (unsigned long)(NO_OF_PERFORMANCE_EVENTS / NO_OF_BITS);

#if defined(BOOST_WINDOWS)
    unsigned long start_time = GetTickCount();
#else
    std::clock_t start_time = std::clock();
#endif // defined(BOOST_WINDOWS)

    // Execute main performance loop
    typedef boost::mpl::range_c< int, 0, NO_OF_BITS >::type complete_loop_t;
    for (register unsigned long i = 0; i < end; ++i)
    {
        boost::mpl::for_each< complete_loop_t >(static_cast< invoke_sm const& >(invoker));
    }

    // Make the rest event deliveries
    typedef boost::mpl::range_c< int, 0, NO_OF_PERFORMANCE_EVENTS % NO_OF_BITS >::type rest_loop_t;
    boost::mpl::for_each< rest_loop_t >(static_cast< invoke_sm const& >(invoker));

    double test_duration_msec =
#if defined(BOOST_WINDOWS)
        GetTickCount() - start_time;
#else
        (std::clock() - start_time) * 1000.0 / CLOCKS_PER_SEC;
#endif // defined(BOOST_WINDOWS)

    std::cout << "Test finished in " << std::fixed << std::setprecision(0)
        << test_duration_msec << " ms (" << std::setprecision(3)
        << (double)(NO_OF_PERFORMANCE_EVENTS) * 1000.0 / test_duration_msec
        << " events/sec)" << std::endl;
}

//////////////////////////////////////////////////////////////////////////
//! Main function
//////////////////////////////////////////////////////////////////////////
int main()
{
    // The array of trampolines to pass different events to the machine in run time
    trampoline_t trampolines[NO_OF_BITS];

    // Initialize trampolines
#define MAKE_TRAMPOLINE(z, iter, data) trampolines[iter] = &invoke_sm::trampoline< iter >;
    BOOST_PP_REPEAT(NO_OF_BITS, MAKE_TRAMPOLINE, ~)
#undef MAKE_TRAMPOLINE

    // Print usage
    std::cout << "Boost.FSM BitMachine example\n";
    std::cout << "Machine configuration: " << (unsigned int)(NO_OF_STATES)
        << " states interconnected with "
#ifndef NO_TRANSITION_MAPS
#ifndef FORCE_MULTIPLE_TRANSITIONS
        << "a single template transition\n\n";
#else
        << (unsigned int)(NO_OF_BITS) << " transitions.\n\n";
#endif // FORCE_MULTIPLE_TRANSITIONS
#else
        << "event handlers (no transition maps).\n\n";
#endif // NO_TRANSITION_MAPS

    // Print usage
    for (unsigned int bit = 0; bit < NO_OF_BITS; ++bit)
    {
        std::cout << bit << "<CR>: Flips bit " << bit << "\n";
    }

    std::cout << "a<CR>: Goes through all states automatically\n";
    std::cout << "p<CR>: Starts a performance test for " << (unsigned long)(NO_OF_PERFORMANCE_EVENTS) << " events\n";
    std::cout << "e<CR>: Exits the program\n\n";
    std::cout << "You may chain commands, e.g. 31<CR> flips bits 3 and 1\n" << std::endl;

    BitFSM_t fsm;
    print_current_state_name(fsm);

    // Begin the test
    char key;
    std::cin >> key;

    while (key != 'e')
    {
        if ((key >= '0') && (key < static_cast< char >('0' + NO_OF_BITS)))
        {
            trampolines[key - '0'](fsm);
        }
        else
        {
            switch (key)
            {
            case 'a':
            {
                for (unsigned int i = 0; i < NO_OF_BITS; ++i)
                {
                    trampolines[i](fsm);
                }
            }
            break;

            case 'p':
            {
                run_performance_test(fsm);
            }
            break;

            default:
                std::cout << "Invalid key!" << std::endl;
            }
        }

        print_current_state_name(fsm);
        std::cin >> key;
    }

    return 0;
}
