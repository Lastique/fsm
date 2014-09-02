/*!
* (C) 2006 Andrey Semashev
* 
* \file   events.cpp
* \author Andrey Semashev
* \date   21.11.2006
* 
* \brief  Tests of simplified events making mechanism
*/

#include "stdafx.hpp"
#include <boost/ref.hpp>
#include <boost/fsm/event.hpp>
#include "boost_testing_helpers.hpp"

namespace EventsTest {

	// Lets define operations that we would like to process in the state machine
	// These are merely needed for tagging the events
	struct Add;
	struct Substract;
	struct Multiply;
	struct Divide;
	// We may also use integral constants to tag events
	enum
	{
		Memorize,
		GetMemory
	};

	// Forward-declaration of state classes
	struct Calculating;
	struct Overflow;

	// Definition of states type list (note we may use any mpl sequence)
	typedef boost::mpl::vector<
		Calculating,
		Overflow
	>::type StatesList_t;

	// A class with common data
	struct CommonData
	{
		int Memory;

		CommonData() : Memory(0) {}
	};

	// We can create a base class for some states that will implement
	// common events processing routines. All we need is to inherit
	// states from this class and import its on_process methods via using-declaration.
	// Moreover, we are permitted to return something from the state machine.
	// In this case each on_process must return a value, this value will be returned
	// from the state machine's process method.
	template< typename StateT >
	struct CommonState :
		// We will return int's from the state machine
		public fsm::state< StateT, StatesList_t, int >,
		// And each state will have access to memory
		virtual public CommonData
	{
		// Every state will be able to process this type of events
		int on_process(fsm::event_c< Memorize, int > const& evt)
		{
			Memory = evt.get< 0 >();
			this->BOOST_NESTED_TEMPLATE switch_to< Calculating >();
			check_bounds();
			return Memory;
		}
		// We can also pass references in events
		int on_process(fsm::event_c< GetMemory, int& > const& evt)
		{
			evt.BOOST_NESTED_TEMPLATE get< 0 >() = Memory;
			return Memory;
		}

		void check_bounds()
		{
			if (Memory > 100 || Memory < -100)
				this->BOOST_NESTED_TEMPLATE switch_to< Overflow >();
		}
	};

	// Implementation of Calculating
	struct Calculating :
		public CommonState< Calculating >
	{
		// We may enforce the state machine to expect any types of events
		// Try to comment out any event handlers to get compile-time error in this line
		BOOST_FSM_MUST_HANDLE_ALL_EVENTS();

		using CommonState< Calculating >::on_process;

		int on_process(fsm::event< Add, int > const& evt)
		{
			Memory += evt.get< 0 >();
			check_bounds();
			return Memory;
		}
		int on_process(fsm::event< Substract, int > const& evt)
		{
			Memory -= evt.get< 0 >();
			check_bounds();
			return Memory;
		}
		int on_process(fsm::event< Multiply, int > const& evt)
		{
			Memory *= evt.get< 0 >();
			check_bounds();
			return Memory;
		}
		int on_process(fsm::event< Divide, int > const& evt)
		{
			Memory /= evt.get< 0 >();
			check_bounds();
			return Memory;
		}
	};

	// Implementation of Overflow
	struct Overflow :
		public CommonState< Overflow >
	{
	};

	// State machine type declaration
	typedef fsm::state_machine< StatesList_t, int > StreamCalc_t;

} // namespace EventsTest

using namespace EventsTest;

BOOST_AUTO_TEST_CASE(simplified_events_support)
{
	TEST_ENTER(simplified_events_support);

	StreamCalc_t calc;

	int result = calc.process(fsm::make_event< Add >(10));
	TEST_REQUIRE(result == 10);
	TEST_REQUIRE(calc.is_in_state< Calculating >());

	result = calc.process(fsm::make_event< Multiply >(1000));
	TEST_REQUIRE(calc.is_in_state< Overflow >());

	result = calc.process(fsm::make_event< Memorize >(-8));
	TEST_REQUIRE(calc.is_in_state< Calculating >());
	TEST_REQUIRE(result == -8);

	result = calc.process(fsm::make_event< Divide >(-2));
	TEST_REQUIRE(calc.is_in_state< Calculating >());
	TEST_REQUIRE(result == 4);

	// We can pass references in events
	int result2 = 0;
	calc.process(fsm::make_event< GetMemory >(boost::ref(result2)));
	TEST_REQUIRE(result == result2);
}
