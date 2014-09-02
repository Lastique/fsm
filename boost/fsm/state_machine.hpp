/*!
 * (C) 2006 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   state_machine.hpp
 * \author Andrey Semashev
 * \date   18.11.2006
 * 
 * \brief  This header is the Boost.FSM library implementation, see the library documentation
 *         at http://www.boost.org/libs/state_machine/doc/state_machine.html
 */

#if (defined(_MSC_VER) && _MSC_VER > 1020)
#pragma once
#endif // _MSC_VER > 1020

#ifndef BOOST_FSM_STATE_MACHINE_HPP_INCLUDED_
#define BOOST_FSM_STATE_MACHINE_HPP_INCLUDED_

#include <cstddef>
#include <typeinfo>
#include <boost/throw_exception.hpp>
#include <boost/static_assert.hpp>
#include <boost/function/function3.hpp>
#include <boost/any.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/index_of.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/advance.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/vector/vector0.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/detail/lightweight_call_once.hpp>
#include <boost/fsm/detail/prologue.hpp>
#include <boost/fsm/exceptions.hpp>

namespace boost {

namespace fsm {

namespace aux {

    //  Forward-declarations of some implementation classes to allow friends declarations
    template< typename, typename, typename >
    class basic_state;
    template< typename, typename, typename >
    class state_impl;
    template< typename, typename, typename >
    class basic_state_machine;
    template< typename, typename >
    struct states_compound;

    //! This class is the most base for every state
    struct BOOST_FSM_NO_VTABLE state_root
    {
        //! Virtual destructor for safety
        virtual ~state_root() {}
        //! Enter state handler for dynamic switch_to support
        virtual void on_enter_state() = 0;
    };

    //! An internal structure that holds information about a single state
    struct state_info
    {
        //! get_state_name function type
        typedef std::string const& (*get_state_name_fun_t)();

        //! Pointer shift between a pointer to state_root of a state
        //! and a pointer to state_machine_root of a complete state machine
        std::ptrdiff_t Shift;
        //! A pointer to type info of a state
        std::type_info const* pTypeInfo;
        //! A pointer to get_state_name function
        get_state_name_fun_t pGetStateName;

        //! Constructor
        state_info() : Shift(0), pTypeInfo(NULL), pGetStateName(NULL) {}
    };

    //! The ultimate base class of a complete states compound. Every state virtually inherits this class.
    template< unsigned int StatesCountV, typename RetValT >
    class BOOST_FSM_NO_VTABLE state_machine_root
    {
    public:
        //! State machine's return type
        typedef RetValT return_type;
        //! Number of states in the state machine
        BOOST_STATIC_CONSTANT(unsigned int, states_count = StatesCountV);

    private:
        //! An internal type that can not be used by user and that cannot be implicitly constructed from anything else
        enum private_type {};

    private:
        //! Current state identifier
        state_id_t m_CurrentState;
        //! A pointer to array of information about states. The pointer is set right after construction.
        const state_info* m_pStatesInfo;
        //! This function is called on unexpected event discovery. If it is empty the default logic will be used.
        function3< return_type, any const&, std::type_info const&, state_id_t > m_UnexpectedEventHandler;

    public:
        //! Default constructor
        state_machine_root() : m_CurrentState(0), m_pStatesInfo(NULL)
        {
        }
        //! Virtual destructor for safety
        virtual ~state_machine_root() {}

        /*!
        *    \brief The method returns current state identifier
        *    \throw None
        */
        state_id_t get_current_state_id() const
        {
            return m_CurrentState;
        }
        /*!
        *    \brief The method returns current state type info
        *    \throw None
        */
        std::type_info const& get_current_state_type() const
        {
            return (*m_pStatesInfo[m_CurrentState].pTypeInfo);
        }
        /*!
        *    \brief The method returns a state type info
        *    \param state_id The identifier of the state
        *    \throw bad_state_id if the state_id argument is invalid
        */
        std::type_info const& get_state_type(state_id_t state_id) const
        {
            if (state_id < states_count)
                return (*m_pStatesInfo[state_id].pTypeInfo);
            else
                throw_exception(bad_state_id(state_id, get_current_state_name(), get_current_state_type(), get_current_state_id()));
        }
        /*!
        *    \brief The method returns current state name
        *    \throw Nothing if the appropriate get_state_name function doesn't throw
        */
        std::string const& get_current_state_name() const
        {
            return (*m_pStatesInfo[m_CurrentState].pGetStateName)();
        }
        /*!
        *    \brief The method returns a state name
        *    \param state_id The identifier of the state
        *    \throw bad_state_id if the state_id argument is invalid or anything the appropriate get_state_name might throw
        */
        std::string const& get_state_name(state_id_t state_id) const
        {
            if (state_id < states_count)
                return (*m_pStatesInfo[state_id].pGetStateName)();
            else
                throw_exception(bad_state_id(state_id, get_current_state_name(), get_current_state_type(), get_current_state_id()));
        }

    private:
        /*!
        *    \brief The method sets an unexpected events handler
        *
        *    The handler may be a functor or a pointer to function with the following signature:
        *
        *    return_type (any const&, std::type_info const&, state_id_t);
        *
        *    See the documentation for arguments description. Old handler, if it was, is lost.
        *
        *    \param handler New unexpected events handler.
        *    \throw Nothing if function assignment operator doesn't throw
        */
        template< typename T >
        void set_unexpected_event_handler(T const& handler)
        {
            m_UnexpectedEventHandler = handler;
        }
        /*!
        *    \brief The method resets the unexpected events handler to the default
        *    \throw Nothing if function clear method doesn't throw
        */
        void set_default_unexpected_event_handler()
        {
            m_UnexpectedEventHandler.clear();
        }

    private:
        //! The method sets a pointer to array of states information array. The method is called right after construction.
        void _set_states_info(const state_info* pStatesInfo)
        {
            m_pStatesInfo = pStatesInfo;
        }
        //! The method changes current state identifier
        void _set_current_state(state_id_t state_id)
        {
            m_CurrentState = state_id;
        }
        //! The method returns state information by state identifier
        state_info const& _get_state_info(state_id_t state_id) const
        {
            return m_pStatesInfo[state_id];
        }
        //! The method invokes unexpected events handler or throws unexpected_event if no handler is set
        return_type _on_unexpected_event(any const& evt, std::type_info const& state_type, state_id_t state_id)
        {
            if (!m_UnexpectedEventHandler.empty())
                return m_UnexpectedEventHandler(evt, state_type, state_id);
            else
                throw_exception(unexpected_event(evt, get_current_state_name(), state_type, state_id));
        }

        //  This is just a protection against attempts to create a standalone state object
        virtual void _creating_a_separate_state_object_is_prohibited_(private_type) = 0;

        //  Declaring other implementation classes friends
        template< typename, typename, typename >
        friend class basic_state;
        template< typename, typename, typename >
        friend class state_impl;
        template< typename, typename, typename >
        friend class basic_state_machine;
        template< typename, typename >
        friend struct states_compound;
    };

    //! This structure is used to detect unexpected events. It may be constructed from any type.
    struct any_event
    {
        //! Unexpected event object is stored here
        any value;

        //! Constructor
        template< typename T >
        any_event(T const& evt) : value(evt) {}
    };

    //! A base class for every state
    template< typename StateT, typename StateListT, typename RetValT >
    class BOOST_FSM_NO_VTABLE basic_state :
        public state_root,
        virtual public state_machine_root< mpl::size< StateListT >::value, RetValT >
    {
    private:
        //! State machine root type
        typedef state_machine_root< mpl::size< StateListT >::value, RetValT > root_type;
        //! Private type import
        typedef typename root_type::private_type private_type;

        //! MPL-style integral constant with index of state in the states list
        typedef typename mpl::index_of< StateListT, StateT >::type state_index_type;

    protected:
        //! A typedef for the state_impl class to detect unexpected events
        typedef any_event _unexpected_event_holder_type;

    public:
        //! State machine return type import
        typedef typename root_type::return_type return_type;
        /*!
        *    \brief Transitions list from this state (empty by default).
        *
        *    User should override this typedef in the derived state class to define his own transitions.
        */
        typedef mpl::vector0< > transitions_type_list;

        //! States count import
        BOOST_STATIC_CONSTANT(unsigned int, states_count = root_type::states_count);
        //! State identifier
        BOOST_STATIC_CONSTANT(state_id_t, state_id = state_index_type::value);

    private:
        //! A pointer to the default state name
        static std::string const& g_DefaultStateName;

    public:
        /*!
        *    \brief The method performs a transition to another state (static version)
        *    \throw Nothing unless on_enter_state or on_leave_state throws
        */
        template< typename AnotherStateT >
        void switch_to()
        {
            typedef typename mpl::index_of< StateListT, AnotherStateT >::type next_state_index_type;
            const state_id_t next_state_id = next_state_index_type::value;

#if defined(_MSC_VER)
#pragma warning(push)
// conditional expression is constant
#pragma warning(disable: 4127)
#endif // defined(_MSC_VER)

            if (next_state_id != state_id)

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // defined(_MSC_VER)

            {
                // Notify the current state about leaving
                register StateT* const pThis = static_cast< StateT* >(this);
                pThis->on_leave_state();
                // Notify the target state about entering
                state_info const& info = root_type::_get_state_info(next_state_id);
                register AnotherStateT* const pThat = static_cast< AnotherStateT* >(
                    reinterpret_cast< state_root* >(
                    reinterpret_cast< char* >(static_cast< root_type* >(this)) + info.Shift));
                // Avoid calling handler virtually
                pThat->AnotherStateT::on_enter_state();
                // Change current state
                root_type::_set_current_state(next_state_id);
            }
        }

        /*!
        *    \brief The method performs a transition to another state (dynamic version)
        *    \param next_state_id Target state identifier
        *    \throw bad_state_id if next_state_id is not valid or anything on_enter_state or on_leave_state might throw
        */
        void switch_to(state_id_t next_state_id)
        {
            if (next_state_id != state_id)
            {
                if (next_state_id < states_count)
                {
                    // Notify the current state about leaving
                    register StateT* const pThis = static_cast< StateT* >(this);
                    pThis->on_leave_state();
                    // Notify the target state about entering
                    state_info const& info = root_type::_get_state_info(next_state_id);
                    register state_root* const pThat =
                        reinterpret_cast< state_root* >(
                        reinterpret_cast< char* >(static_cast< root_type* >(this)) + info.Shift);
                    pThat->on_enter_state();
                    // Change current state
                    root_type::_set_current_state(next_state_id);
                }
                else
                {
                    // Invalid state identifier detected
                    throw_exception(bad_state_id(next_state_id, root_type::get_current_state_name(), typeid(StateT), state_id));
                }
            }
        }

        //! Default implementation of state enter handler to support its optionality
        void on_enter_state() {}
        //! Default implementation of state leave handler to support its optionality
        void on_leave_state() {}
        //! Default implementation of state machine reset handler to support its optionality
        void on_reset() {}

        //! An additional on_process method that will never be called since nothing can be converted to private_type.
        //! This declaration is needed to make using declaration in state_impl valid even if there is no
        //! on_process handlers in the state.
        return_type on_process(private_type);

        //! An accessor to the default state name
        static std::string const& get_state_name()
        {
            // This is thread-safe, since the reference have been initialized from make_state_name
            // that was already called from dynamic_initialization
            return g_DefaultStateName;
        }

    private:
        //! The method performs dynamic state initialization
        static std::string const& dynamic_initialization()
        {
            static detail::lw_call_once::call_once_trigger trigger = BOOST_LWCO_INIT;
            detail::lw_call_once::call_once(trigger, &basic_state::make_state_name);
            return make_state_name();
        }

        //! A state name construction helper
        static std::string const& make_state_name()
        {
            static const std::string name(construct_type_name(typeid(StateT)));
            return name;
        }

        template< typename, typename, typename >
        friend class basic_state_machine;
    };

    //! A pointer to the default state name
    template< typename StateT, typename StateListT, typename RetValT >
    std::string const& basic_state<
        StateT,
        StateListT,
        RetValT
    >::g_DefaultStateName = basic_state< StateT, StateListT, RetValT >::dynamic_initialization();

    //! A super-class for state that detects unexpected events
    template< typename StateT, typename StateListT, typename RetValT >
    class BOOST_FSM_NO_VTABLE state_impl :
        // To protect against cross-casting between states we inherit the state protected
        protected StateT
    {
    private:
        //! State type
        typedef StateT state_type;
        //! State base type
        typedef basic_state< StateT, StateListT, RetValT > base_type;
        //! State machine root type
        typedef state_machine_root< mpl::size< StateListT >::value, RetValT > root_type;
        //! A typedef for the state_impl class to detect unexpected events
        typedef typename state_type::_unexpected_event_holder_type unexpected_event_holder_type;

        //  Static check for that user correctly filled fsm::state's template parameters
        BOOST_STATIC_ASSERT((is_base_and_derived< base_type, state_type >::value));

    public:
        //! State machine return type import
        typedef typename base_type::return_type return_type;

    public:
        //  on_process handlers import from state class
        using state_type::on_process;

        //! This on_process handler will be called if no appropriate handler found in the state
        return_type on_process(unexpected_event_holder_type const& evt)
        {
            return root_type::_on_unexpected_event(evt.value, typeid(StateT), base_type::state_id);
        }
    };

//! The macro allows to enforce all possible events support in the state
#define BOOST_FSM_MUST_HANDLE_ALL_EVENTS()\
    private:\
        struct THE_STATE_SHOULD_SUPPORT_THIS_EVENT_TYPE\
        {\
            boost::any value;\
            template< typename EventT >\
            THE_STATE_SHOULD_SUPPORT_THIS_EVENT_TYPE(EventT const&)\
            {\
                BOOST_STATIC_ASSERT((!boost::is_same< EventT, EventT >::value));\
            }\
        };\
    public:\
        typedef THE_STATE_SHOULD_SUPPORT_THIS_EVENT_TYPE _unexpected_event_holder_type


    //! A proxy predicate used to find an applicable transition in the transition map
    template< typename StateT, typename EventT >
    struct applicable_transition_pred
    {
        template< typename TransitionT >
        struct apply :
            public TransitionT::BOOST_NESTED_TEMPLATE is_applicable< StateT, EventT >
        {
        };
    };


    //! An auxiliary structure that contains friendly functions for state machine implementation
    struct state_machine_access
    {
        //! The method fills dispatching map element for the case when no transition is to be performed
        template< typename StateMachineT, typename StateT, typename TransitionItT, typename EventT, typename ProcessFuncsT >
        BOOST_FSM_FORCEINLINE static void do_init_process_functions(ProcessFuncsT* process_funcs, mpl::true_ const&)
        {
            // Here we can eliminate unnecessary calls to perform_transition later in run-time
            // since we know that there's no transition in the map.
            // The "second" still needs to be valid because there may exist automatic
            // transitions to this state, and the "second" part of the pair will be used
            // in perform_transition method of that transition.
            process_funcs->second = process_funcs->first =
                &StateMachineT::BOOST_NESTED_TEMPLATE deliver_event< StateT, EventT >;
        }
        //! The method fills dispatching map element for the case when a transition has to be performed
        template< typename StateMachineT, typename StateT, typename TransitionItT, typename EventT, typename ProcessFuncsT >
        BOOST_FSM_FORCEINLINE static void do_init_process_functions(ProcessFuncsT* process_funcs, mpl::false_ const&)
        {
            process_funcs->first = &StateMachineT::BOOST_NESTED_TEMPLATE perform_transition<
                StateT, typename mpl::deref< TransitionItT >::type, EventT >;
            process_funcs->second = &StateMachineT::BOOST_NESTED_TEMPLATE deliver_event< StateT, EventT >;
        }
    };

    /*!
    *    \brief An internal class that recursively inherits all states
    *
    *    This implementation unrolls up to 5 inheritance steps to reduce the amount of
    *    generated code (structors, assignment, type_info and vtable on non-MS compilers). This may also
    *    speed up the compilation of larger machines a little.
    */
    template< typename StateListT, typename RetValT, typename StatesListIterT, unsigned int SizeV >
    struct BOOST_FSM_NO_VTABLE inherited_states :
        public state_impl< typename mpl::deref< StatesListIterT >::type, StateListT, RetValT >,
        public state_impl< typename mpl::deref< typename mpl::advance_c< StatesListIterT, 1 >::type >::type, StateListT, RetValT >,
        public state_impl< typename mpl::deref< typename mpl::advance_c< StatesListIterT, 2 >::type >::type, StateListT, RetValT >,
        public state_impl< typename mpl::deref< typename mpl::advance_c< StatesListIterT, 3 >::type >::type, StateListT, RetValT >,
        public state_impl< typename mpl::deref< typename mpl::advance_c< StatesListIterT, 4 >::type >::type, StateListT, RetValT >,
        public inherited_states< StateListT, RetValT, typename mpl::advance_c< StatesListIterT, 5 >::type, SizeV - 5 >
    {
        //! Other inherited states
        typedef inherited_states< StateListT, RetValT, typename mpl::advance_c< StatesListIterT, 5 >::type, SizeV - 5 > rest_states;
        //! State machine return type
        typedef RetValT return_type;

        //! First state type
        typedef typename mpl::deref< StatesListIterT >::type state1_type;
        //! First state super-class
        typedef state_impl< state1_type, StateListT, RetValT > state1_impl_type;
        //! Second state type
        typedef typename mpl::deref< typename mpl::advance_c< StatesListIterT, 1 >::type >::type state2_type;
        //! Second state super-class
        typedef state_impl< state2_type, StateListT, RetValT > state2_impl_type;
        //! 3rd state type
        typedef typename mpl::deref< typename mpl::advance_c< StatesListIterT, 2 >::type >::type state3_type;
        //! 3rd state super-class
        typedef state_impl< state3_type, StateListT, RetValT > state3_impl_type;
        //! 4th state type
        typedef typename mpl::deref< typename mpl::advance_c< StatesListIterT, 3 >::type >::type state4_type;
        //! 4th state super-class
        typedef state_impl< state4_type, StateListT, RetValT > state4_impl_type;
        //! 5th state type
        typedef typename mpl::deref< typename mpl::advance_c< StatesListIterT, 4 >::type >::type state5_type;
        //! 5th state super-class
        typedef state_impl< state5_type, StateListT, RetValT > state5_impl_type;

        //! The method recursively invokes on_reset handlers for all states
        BOOST_FSM_FORCEINLINE void on_reset()
        {
#define BOOST_PP_ITERATION_LIMITS (1, 5)
#define BOOST_PP_FILENAME_1 <boost/fsm/detail/inh_st_on_reset.hpp>
#include BOOST_PP_ITERATE()
            rest_states::on_reset();
        }

        /*!
        *    \brief The method fills the state information array
        *    \param pRoot reinterpret_cast'ed pointer to state_machine_root of this state machine
        *    \param pStateInfo pointer to state info to fill
        */
        BOOST_FSM_FORCEINLINE void init_states_info(const char* pRoot, volatile state_info* pStateInfo) const
        {
            BOOST_FSM_ASSUME(this != NULL);
#define BOOST_PP_ITERATION_LIMITS (1, 5)
#define BOOST_PP_FILENAME_1 <boost/fsm/detail/inh_st_init_states_info.hpp>
#include BOOST_PP_ITERATE()
            rest_states::init_states_info(pRoot, pStateInfo);
        }

        //! The method recursively initializes the dispatching map
        template< typename StateMachineT, typename EventT, typename ProcessFuncsT >
        static BOOST_FSM_FORCEINLINE void init_process_functions(ProcessFuncsT* process_funcs)
        {
#define BOOST_PP_ITERATION_LIMITS (1, 5)
#define BOOST_PP_FILENAME_1 <boost/fsm/detail/inh_st_init_process_functions.hpp>
#include BOOST_PP_ITERATE()
            // Fill other elements
            rest_states::BOOST_NESTED_TEMPLATE init_process_functions<
                StateMachineT, EventT, ProcessFuncsT
            >(process_funcs);
        }
    };

//  Make specializations for 1 - 5 states in the list
#define BOOST_PP_ITERATION_LIMITS (1, 5)
#define BOOST_PP_FILENAME_1 <boost/fsm/detail/inherited_states.hpp>
#include BOOST_PP_ITERATE()

    //! A specialization for empty states list, this should not compile
    template< typename StateListT, typename RetValT >
    struct inherited_states< StateListT, RetValT, typename mpl::end< StateListT >::type, 0 >;


    //! A compound class that contains all states
    template< typename StateListT, typename RetValT >
    struct states_compound :
        public inherited_states<
            StateListT,
            RetValT,
            typename mpl::begin< StateListT >::type,
            mpl::size< StateListT >::value
        >
    {
    private:
        //! Base type
        typedef inherited_states<
            StateListT,
            RetValT,
            typename mpl::begin< StateListT >::type,
            mpl::size< StateListT >::value
        > base_type;
        //! State machine root type
        typedef state_machine_root< mpl::size< StateListT >::value, RetValT > root_type;

    public:
        //! The method fills the state information array
        BOOST_FSM_NOINLINE void init_states_info(volatile state_info* pStates) const
        {
            base_type::init_states_info(
                reinterpret_cast< const char* >(static_cast< const root_type* >(this)), pStates);
        }

    private:
        //  This is just a protection against attempts to create a standalone state object
        void _creating_a_separate_state_object_is_prohibited_(typename root_type::private_type) {}

        //  This friend declaration is needed to have the ability to cast
        //  pointers and references to states_compound object to pointers and references
        //  to states and their bases, including state_machine_root
        template< typename, typename, typename >
        friend class basic_state_machine;
    };


    //! A class used to dispatch a call to state machine's process method depending on its current state
    template< typename EventT, typename StateMachineT >
    class state_dispatcher
    {
    private:
        //! State machine base class
        typedef StateMachineT state_machine_type;
        //! The event type
        typedef EventT event_type;
        //! State machine return type
        typedef typename state_machine_type::return_type return_type;
        //! States compound type
        typedef typename state_machine_type::states_compound_type states_compound_type;
        //! Function type used to process event in a single state
        typedef return_type (BOOST_FSM_FASTCALL* process_fun_t)(states_compound_type&, event_type const&);
        //! A dispatching map entry type (we need to guarantee that it's POD, so we can't use std::pair here)
        struct entry
        {
            //! A function pointer to be called to execute transition
            process_fun_t first;
            //! A function pointer to be called to deliver the event
            process_fun_t second;
        };

    private:
        //! The array is used to call the process_internal function that corresponds to the current state
        entry m_Entries[state_machine_type::states_count];

        //! The only dispatcher instance
        static state_dispatcher const g_Instance;

    public:
        //! Default constructor
        BOOST_FSM_NOINLINE state_dispatcher()
        {
            // Race condition is possible here if the dynamic initialization
            // of the namespace-scope static variables is performed concurrently.
            // But even if it is executed in multiple threads concurrently
            // the race between threads is not significant since only POD types
            // are involved and the result of initialization does not depend on
            // number of threads or number of initializations.
            states_compound_type::BOOST_NESTED_TEMPLATE init_process_functions<
                state_machine_type, event_type, entry
            >(m_Entries);
        }

        //! The subscript operator is used to get the appropriate function pointer
        BOOST_FSM_FORCEINLINE entry const& operator[] (state_id_t state_id) const
        {
            return m_Entries[state_id];
        }

        //! The method returns a reference to the only dispatcher instance
        static BOOST_FSM_FORCEINLINE state_dispatcher const& get()
        {
            return g_Instance;
        }
    };

    //! Implementation of the state dispatchers
    template< typename EventT, typename StateMachineT >
    state_dispatcher< EventT, StateMachineT > const state_dispatcher< EventT, StateMachineT >::g_Instance;


    //! An implementation of state machine
    template< typename StateListT, typename RetValT, typename TransitionListT >
    class basic_state_machine
    {
    private:
        //! Declare friendly functions
        friend struct state_machine_access;
        //! A class used to dispatch a call to process method depending on the current state (declaring as a friend)
        template< typename, typename >
        friend class state_dispatcher;

        //! Self type
        typedef basic_state_machine this_type;

#if defined(__GNUC__)
    // GCC doesn't see the friend declaration of state_dispatcher above
    public:
#endif // defined(__GNUC__)
        //! A type that inherits all states
        typedef states_compound< StateListT, RetValT > states_compound_type;

    public:
        //! States type sequence
        typedef StateListT states_type_list;
        //! State machine return type
        typedef RetValT return_type;
        //! Transitions map type sequence. To shorten symbol names it is void by default in template parameters.
        typedef typename mpl::if_<
            is_same< TransitionListT, void >,
            mpl::vector0< >,
            TransitionListT
        >::type transitions_type_list;

        //! States count
        BOOST_STATIC_CONSTANT(unsigned int, states_count = mpl::size< states_type_list >::value);

    protected:
        //! State machine root type (protected only to allow library extensions access the type)
        typedef state_machine_root< states_count, return_type > root_type;

    private:
        //! States information holder (declaring as a friend)
        class states_info_holder;
        friend class states_info_holder;

        //! States information holder
        class states_info_holder
        {
        private:
            //! An array with information about states
            state_info m_StatesInfo[states_count];
            //! A flag that shows if the array is initialized
            volatile bool m_fInitialized;

        public:
            //! Default constructor
            states_info_holder() : m_fInitialized(false)
            {
            }

            //! An initialization method. Fills m_StatesInfo array on the first call.
            void init(const states_compound_type* pStates)
            {
                // Race condition is possible here, but it is not significant
                // since only POD types are involved and the result of initialization
                // does not depend on number of threads or number of initializations.
                if (!m_fInitialized)
                {
                    // Fill m_StatesInfo array for all states
                    pStates->init_states_info(m_StatesInfo);
                    m_fInitialized = true;
                }
            }

            //! An accessor to the array with information about states
            const state_info* get() const { return m_StatesInfo; }
        };

    private:
        //! An object that contains all states
        states_compound_type m_States;
        //! Global holder of the information about states
        static states_info_holder g_StatesInfoHolder;

    public:
        /*!
        *    \brief Default constructor
        *    \throw Nothing unless a state constructor throws
        */
        basic_state_machine()
        {
            BOOST_FSM_ASSUME(&m_States != NULL);

            // State information initialization (runs only once)
            g_StatesInfoHolder.init(&m_States);
            root_type& Root = m_States;
            Root._set_states_info(g_StatesInfoHolder.get());
        }
        /*!
        *    \brief A constructor with automatic unexpected events handler setting
        *    \param handler Unexpected event handler, see set_unexpected_event_handler comment.
        *    \throw Nothing unless a state constructor or set_unexpected_event_handler throws
        */
        template< typename T >
        basic_state_machine(T const& handler)
        {
            BOOST_FSM_ASSUME(&m_States != NULL);

            // State information initialization (runs only once)
            g_StatesInfoHolder.init(&m_States);
            root_type& Root = m_States;
            Root._set_states_info(g_StatesInfoHolder.get());

            // Unexpected event handler setup
            set_unexpected_event_handler(handler);
        }

        /*!
        *    \brief Event processing routine
        *    \param evt The event to pass to state machine
        *    \return The result of on_process handler called or, in case if no handler found, the result of an unexpected event routine
        *    \throw May only throw if an on_process handler throws or, in case if no handler found, if an unexpected event routine throws
        */
        template< typename EventT >
        return_type process(EventT const& evt)
        {
            typedef state_dispatcher< EventT, this_type > dispatcher_type;
            return (dispatcher_type::get()[get_current_state_id()].first)(m_States, evt);
        }

        /*!
        *    \brief The method checks if th state machine is in a specified state
        *    \return true if the state machine is in the state, false otherwise
        *    \throw None
        */
        template< typename StateT >
        bool is_in_state() const
        {
            typedef typename mpl::index_of< states_type_list, StateT >::type state_index_type;
            return (state_index_type::value == get_current_state_id());
        }

        /*!
        *    \brief The method returns a reference to a specified state or its base class
        *    \throw None
        */
        template< typename T >
        T& get() { return static_cast< T& >(m_States); }
        /*!
        *    \brief The method returns a reference to a specified state or its base class
        *    \throw None
        */
        template< typename T >
        T const& get() const { return static_cast< const T& >(m_States); }

        /*!
        *    \brief The method resets the state machine to its initial state
        *    \throw None
        */
        void reset()
        {
            BOOST_FSM_ASSUME(&m_States != NULL);
            m_States.on_reset(); // will not throw
            root_type& Root = m_States;
            Root._set_current_state(state_id_t(0));
        }

        //  Rest of the state machine methods are implemented in the state_machine_root class
        /*!
        *    \brief The method returns current state identifier
        *    \sa state_machine_root::get_current_state_id
        */
        state_id_t get_current_state_id() const
        {
            BOOST_FSM_ASSUME(&m_States != NULL);
            root_type const& Root = m_States;
            return Root.get_current_state_id();
        }
        /*!
        *    \brief The method returns current state type info
        *    \sa state_machine_root::get_current_state_type
        */
        std::type_info const& get_current_state_type() const
        {
            BOOST_FSM_ASSUME(&m_States != NULL);
            root_type const& Root = m_States;
            return Root.get_current_state_type();
        }
        /*!
        *    \brief The method returns a state type info
        *    \sa state_machine_root::get_state_type
        */
        std::type_info const& get_state_type(state_id_t state_id) const
        {
            BOOST_FSM_ASSUME(&m_States != NULL);
            root_type const& Root = m_States;
            return Root.get_state_type(state_id);
        }
        /*!
        *    \brief The method returns current state name
        *    \sa state_machine_root::get_current_state_name
        */
        std::string const& get_current_state_name() const
        {
            BOOST_FSM_ASSUME(&m_States != NULL);
            root_type const& Root = m_States;
            return Root.get_current_state_name();
        }
        /*!
        *    \brief The method returns a state name
        *    \sa state_machine_root::get_state_name
        */
        std::string const& get_state_name(state_id_t state_id) const
        {
            BOOST_FSM_ASSUME(&m_States != NULL);
            root_type const& Root = m_States;
            return Root.get_state_name(state_id);
        }
        /*!
        *    \brief The method sets an unexpected events handler
        *    \sa state_machine_root::set_unexpected_event_handler
        */
        template< typename T >
        void set_unexpected_event_handler(T const& handler)
        {
            BOOST_FSM_ASSUME(&m_States != NULL);
            root_type& Root = m_States;
            Root.set_unexpected_event_handler(handler);
        }
        /*!
        *    \brief The method resets the unexpected events handler to the default
        *    \sa state_machine_root::set_default_unexpected_event_handler
        */
        void set_default_unexpected_event_handler()
        {
            BOOST_FSM_ASSUME(&m_States != NULL);
            root_type& Root = m_States;
            Root.set_default_unexpected_event_handler();
        }

    private:
        //! The method performs automatic transition, if there is one in the transitions map, and passes the event to the state
        template< typename StateT, typename TransitionT, typename EventT >
        static return_type BOOST_FSM_FASTCALL perform_transition(states_compound_type& States, EventT const& Event)
        {
            BOOST_FSM_ASSUME(&States != NULL);

            // Get a reference to current state
            StateT& CurrentState = static_cast< StateT& >(States);

            // Perform the transition
            TransitionT::transit(CurrentState, Event);

            // Since the transition might have changed the state
            // we have to perform second dispatch to deliver the event to the actual state.
            root_type& Root = States;
            typedef state_dispatcher< EventT, this_type > dispatcher_type;
            return (dispatcher_type::get()[Root.get_current_state_id()].second)(States, Event);
        }

        //! The method delivers the event to the state
        template< typename StateT, typename EventT >
        static return_type BOOST_FSM_FASTCALL deliver_event(states_compound_type& States, EventT const& Event)
        {
            BOOST_FSM_ASSUME(&States != NULL);

            // Get a reference to current state
            typedef state_impl< StateT, states_type_list, return_type > current_state_t;
            current_state_t& CurrentState = static_cast< current_state_t& >(States);

            // Invoke event handler
            return CurrentState.on_process(Event);
        }
    };

    //! Implementation of states information holder
    template< typename StateListT, typename RetValT, typename TransitionListT >
    typename basic_state_machine<
        StateListT,
        RetValT,
        TransitionListT
    >::states_info_holder basic_state_machine< StateListT, RetValT, TransitionListT >::g_StatesInfoHolder;

#if !defined(BOOST_NO_INCLASS_MEMBER_INITIALIZATION)

    //  According to 9.4.2/4, even integral static constants with in-class initialization
    //  have to be defined out of the class. We only need to do this if the compiler supports such constants.
    template< unsigned int StatesCountV, typename RetValT >
    const unsigned int state_machine_root< StatesCountV, RetValT >::states_count;
    template< typename StateT, typename StateListT, typename RetValT >
    const unsigned int basic_state< StateT, StateListT, RetValT >::states_count;
    template< typename StateT, typename StateListT, typename RetValT >
    const state_id_t basic_state< StateT, StateListT, RetValT >::state_id;
    template< typename StateListT, typename RetValT, typename TransitionListT >
    const unsigned int basic_state_machine< StateListT, RetValT, TransitionListT >::states_count;

#endif // !defined(BOOST_NO_INCLASS_MEMBER_INITIALIZATION)

} // namespace aux

//! A basic state class for users
template< typename StateT, typename StateListT, typename RetValT = void >
class state :
    public aux::basic_state< StateT, StateListT, RetValT >
{
};

//! A state machine class for users
template< typename StateListT, typename RetValT = void, typename TransitionListT = void >
class state_machine :
    public aux::basic_state_machine< StateListT, RetValT, TransitionListT >
{
    //! Implementation type
    typedef aux::basic_state_machine< StateListT, RetValT, TransitionListT > base_type;

public:
    //! Default constructor
    state_machine() {}
    //! Copying constructor
    state_machine(state_machine const& that) : base_type(static_cast< base_type const& >(that)) {}
    //! A constructor with unexpected events handler setup
    template< typename T >
    state_machine(T const& handler) : base_type(handler) {}
};

} // namespace fsm

} // namespace boost

#endif // BOOST_FSM_STATE_MACHINE_HPP_INCLUDED_
