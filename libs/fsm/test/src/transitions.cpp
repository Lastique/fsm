/*!
* (C) 2006 Andrey Semashev
* 
* \file   transitions.cpp
* \author Andrey Semashev
* \date   21.11.2006
* 
* \brief  Tests for transition map support
*/

#include "stdafx.hpp"
#include <boost/mpl/bool.hpp>
#include <boost/mpl/list.hpp>
#include <boost/fsm/transition.hpp>
#include "boost_testing_helpers.hpp"

namespace TransitionsTest {

	// Event classes
	struct EventBase {};
	struct Event1 : public EventBase {};
	struct Event2 : public EventBase {};
	template< typename FieldT >
	struct Event3
	{
		typedef FieldT type;
		FieldT value;
		Event3(FieldT const& val) : value(val)
		{
		}
	};
	struct StraightToEnd {};

	// Forward-declaration of state classes
	struct InitialState;
	struct State1;
	struct State2;
	struct FinalState;

	// Definition of states type list (note we may use any mpl sequence)
	typedef boost::mpl::list<
		InitialState,
		State1,
		State2,
		FinalState
	>::type StatesList_t;

	// Implementation of InitialState
	struct InitialState :
		public fsm::state< InitialState, StatesList_t >
	{
		// We may break the transitions map into several pieces
		// Here, in each state, we may define transitions from this state
		// to other states
		typedef boost::mpl::vector<
			fsm::transition< InitialState, Event1, State1 >,
			fsm::transition< InitialState, Event2, State2 >
		>::type transitions_type_list;
	};

	// Implementation of State1
	struct State1 :
		public fsm::state< State1, StatesList_t >
	{
		// Another state-specific part of the transitions map
		typedef boost::mpl::vector<
			fsm::transition< State1, Event3< double >, State2 >
		>::type transitions_type_list;

		// Event processing methods
		void on_process(EventBase) {}
	};

	// Implementation of State2
	struct State2 :
		public fsm::state< State2, StatesList_t >
	{
		// Event processing methods
		void on_process(Event2) {}

		template< typename T >
		void on_process(Event3< T > const&) {}
	};

	// Implementation of FinalState
	struct FinalState :
		public fsm::state< FinalState, StatesList_t >
	{
		// Event processing methods
		void on_process(StraightToEnd) {}

		template< typename T >
		void on_process(Event3< T > const& evt) {}
	};

	// We can define our own transition rules
	struct my_transition :
		// If the transition is applicable and allowed the next state would be FinalState
		public fsm::basic_transition< FinalState >
	{
		typedef fsm::basic_transition< FinalState > base_type;

		// Compile-time criteria of applicability of this transition rule
		template< typename StateT, typename EventT >
		struct is_applicable : boost::mpl::false_ {};

		// And we can specify a number of run-time checks of the event to make a decision of transition
		template< typename StateT >
		static void transit(StateT& state, Event3< int > const& evt)
		{
			if (evt.value == 10)
				base_type::transit(state, evt);
		}
		template< typename StateT >
		static void transit(StateT& state, Event3< std::string > const& evt)
		{
			if (!evt.value.empty())
				base_type::transit(state, evt);
		}
		template< typename StateT, typename T >
		static void transit(StateT& state, Event3< T > const& evt)
		{
			// Do nothing, no transition to be performed
		}
	};

	// The transition may take place when state machine is in State2 and receives any instance of Event3
	template< typename T >
	struct my_transition::is_applicable< State2, Event3< T > > :
		public boost::mpl::true_
	{
	};

	// Transitions map - the common part that will be used for each state
	// We could have put all transitions into this single map
	typedef boost::mpl::vector<
		my_transition,
		// And we can specify transitions that are applicable regardless of the current state
		// But such transitions are better to be at the end of the list because the applicable
		// transition is being looked for from begin to end, and once it is found the engine
		// will seek no further.
		fsm::transition< fsm::any_state, StraightToEnd, FinalState >
	>::type TransitionsMap_t;

	// State machine type declaration
	typedef fsm::state_machine< StatesList_t, void, TransitionsMap_t > StateMachine_t;

} // namespace TransitionsTest

using namespace TransitionsTest;


BOOST_AUTO_TEST_CASE(state_transitions)
{
	TEST_ENTER(state_transitions);

	StateMachine_t fsm;
	TEST_REQUIRE(fsm.is_in_state< InitialState >());

	fsm.process(Event1()); // switches to State1
	TEST_REQUIRE(fsm.is_in_state< State1 >());

	fsm.process(Event3< double >(3.3)); // switches to State2
	TEST_REQUIRE(fsm.is_in_state< State2 >());

	fsm.process(Event2()); // does not switch (no transition applies)
	TEST_REQUIRE(fsm.is_in_state< State2 >());

	fsm.process(Event3< std::string >("")); // does not switch (run-time check returns false)
	TEST_REQUIRE(fsm.is_in_state< State2 >());

	fsm.process(Event3< int >(10)); // switches to FinalState
	TEST_REQUIRE(fsm.is_in_state< FinalState >());
}

BOOST_AUTO_TEST_CASE(any_state_support)
{
	TEST_ENTER(any_state_support);

	StateMachine_t fsm;
	TEST_REQUIRE(fsm.is_in_state< InitialState >());

	fsm.process(StraightToEnd()); // switches to FinalState
	TEST_REQUIRE(fsm.is_in_state< FinalState >());


	// Lets try another state
	fsm.reset();
	TEST_REQUIRE(fsm.is_in_state< InitialState >());

	fsm.process(Event2()); // switches to State2
	TEST_REQUIRE(fsm.is_in_state< State2 >());

	fsm.process(StraightToEnd()); // once again, switches to FinalState
	TEST_REQUIRE(fsm.is_in_state< FinalState >());
}
