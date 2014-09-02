/*!
* (C) 2006 Andrey Semashev
* 
* \file   locking.cpp
* \author Andrey Semashev
* \date   24.12.2006
* 
* \brief  Locking state machines functionality tests
*/

#include "stdafx.hpp"
// #include <boost/thread/mutex.hpp>
#include <boost/fsm/locking_state_machine.hpp>
#include "boost_testing_helpers.hpp"

namespace LockingTest {

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

	// Forward-declaration of state classes
	struct InitialState;
	struct State1;
	struct State2;
	struct FinalState;

	// Definition of states type list
	typedef boost::mpl::vector<
	    InitialState,
	    State1,
		State2,
		FinalState
	>::type StatesList_t;

	// We may create a class that holds common data and methods of the whole state machine
	// We just have to virtually inherit each state from this class
	struct CommonData
	{
		std::string m_EventsTrace;

		template< typename T >
		void trace()
		{
			if (!m_EventsTrace.empty())
				m_EventsTrace.append(", ");
			m_EventsTrace.append(typeid(T).name());
		}
	};

	// Implementation of InitialState
	struct InitialState :
		public fsm::state< InitialState, StatesList_t >,
		virtual public CommonData
	{
		bool m_Event1Received;
		bool m_Event2Received;

		// Constructor
        InitialState() : m_Event1Received(false), m_Event2Received(false) {}

		// Event processing methods
		void on_process(Event1 const& evt)
		{
			trace< Event1 >();
            m_Event1Received = true;
            switch_to< State1 >();
		}
		void on_process(Event2 const& evt)
		{
			trace< Event2 >();
            m_Event2Received = true;
            switch_to< State2 >();
		}

		// This method will be called on state machine reset
		void on_reset()
		{
            m_Event1Received = m_Event2Received = false;
			m_EventsTrace.clear();
		}

		// We can alter state name, the default would be a bit enhanced state type name
		static std::string const& get_state_name()
		{
			static const std::string name = "Initial state";
			return name;
		}
	};

	// Implementation of State1
	struct State1 :
		public fsm::state< State1, StatesList_t >,
		virtual public CommonData
	{
		bool m_EventBaseReceived;
		bool m_Event3Received;
		boost::any m_Event3Value;

		// Constructor
        State1() : m_EventBaseReceived(false), m_Event3Received(false) {}

		// Event processing methods
		// Event processing methods can be templates
		template< typename T >
		void on_process(Event3< T > const& evt)
		{
			trace< Event3< T > >();
            m_Event3Received = true;
            m_Event3Value = evt.value;
            switch_to< State2 >();
		}
		// Event processing methods may require an implicit standard conversion of event type (but not user-defined)
		void on_process(EventBase const& evt)
		{
			trace< EventBase >();
            m_EventBaseReceived = true;
            switch_to< FinalState >();
		}

		// This method will be called on state machine reset
		void on_reset()
		{
            m_EventBaseReceived = m_Event3Received = false;
            m_Event3Value = boost::any();
		}
	};

	// Implementation of State2
	struct State2 :
		public fsm::state< State2, StatesList_t >,
		virtual public CommonData
	{
		bool m_OnEnterStateReceived;
		bool m_OnLeaveStateReceived;

		// Constructor
        State2() : m_OnEnterStateReceived(false), m_OnLeaveStateReceived(false) {}

		// Event processing methods
		template< typename T >
		void on_process(T const& evt);

		// This method will be called when the state is entered
		void on_enter_state()
		{
            m_OnEnterStateReceived = true;
		}
		// This method will be called when the state is left
		void on_leave_state()
		{
            m_OnLeaveStateReceived = true;
		}

		// This method will be called on state machine reset
		void on_reset()
		{
            m_OnEnterStateReceived = m_OnLeaveStateReceived = false;
		}
	};

	// Implementation of FinalState
	struct FinalState :
		public fsm::state< FinalState, StatesList_t >,
		virtual public CommonData
	{
		// This handler is just for testing purposes only
		void on_process(Event3< fsm::state_id_t > const& evt)
		{
			trace< Event3< fsm::state_id_t > >();
			switch_to(evt.value);
		}
	};

	// Event processing methods
	template< typename T >
	void State2::on_process(T const& evt)
	{
		trace< T >();
		// We may also use state id's to switch between states
		// though this requires FinalState to be defined at this point
        switch_to(FinalState::state_id);
	}

	// State machine type declaration
	typedef fsm::locking_state_machine< StatesList_t > StateMachine_t;

} // namespace LockingTest

using namespace LockingTest;


BOOST_AUTO_TEST_CASE(locking_event_delivery)
{
	TEST_ENTER(locking_event_delivery);

	StateMachine_t fsm;
	TEST_REQUIRE(fsm.is_in_state< InitialState >());

	fsm.process(Event1()); // switches to State1
	fsm.process(Event2()); // switches to FinalState
	TEST_REQUIRE(fsm.is_in_state< FinalState >());

	InitialState const& inital_state = fsm.get< InitialState >();
	TEST_REQUIRE(inital_state.m_Event1Received);
	TEST_REQUIRE(!inital_state.m_Event2Received);

	State1 const& state1 = fsm.get< State1 >();
	TEST_REQUIRE(state1.m_EventBaseReceived);
	TEST_REQUIRE(!state1.m_Event3Received);
}

BOOST_AUTO_TEST_CASE(locking_enter_leave_reset)
{
	TEST_ENTER(locking_enter_leave_reset);

	StateMachine_t fsm;
	TEST_REQUIRE(fsm.is_in_state< InitialState >());

	fsm.process(Event1()); // switches to State1
	fsm.process(Event3< std::string >("yo-ho-ho")); // switches to State2

	TEST_REQUIRE(fsm.is_in_state< State2 >());
	State2 const& state2 = fsm.get< State2 >();
	TEST_REQUIRE(state2.m_OnEnterStateReceived);
	TEST_REQUIRE(!state2.m_OnLeaveStateReceived);

	fsm.process(Event1()); // switches to FinalState
	TEST_REQUIRE(fsm.is_in_state< FinalState >());

	InitialState const& inital_state = fsm.get< InitialState >();
	TEST_REQUIRE(inital_state.m_Event1Received);
	TEST_REQUIRE(!inital_state.m_Event2Received);

	State1 const& state1 = fsm.get< State1 >();
	TEST_REQUIRE(state1.m_Event3Received);
	TEST_REQUIRE(!state1.m_EventBaseReceived);
	std::string str = boost::any_cast< std::string >(state1.m_Event3Value);
	TEST_REQUIRE(str == "yo-ho-ho");

	TEST_REQUIRE(state2.m_OnLeaveStateReceived);

	// Test reset
	fsm.reset();
	TEST_REQUIRE(fsm.is_in_state< InitialState >());
	TEST_REQUIRE(!inital_state.m_Event1Received);
	TEST_REQUIRE(!inital_state.m_Event2Received);
	TEST_REQUIRE(!state1.m_EventBaseReceived);
	TEST_REQUIRE(!state1.m_Event3Received);
	TEST_REQUIRE(!state2.m_OnEnterStateReceived);
	TEST_REQUIRE(!state2.m_OnLeaveStateReceived);
}

//! This is a custom unexpected event handler that checks the correctness of its arguments
void my_locking_unexpected_event_handler(boost::any const& evt, std::type_info const& state, fsm::state_id_t id)
{
	if (evt.type() != typeid(Event3< int >))
	{
		std::string err = "my_locking_unexpected_event_handler: The event type is not Event3< int >: ";
		err += evt.type().name();
		throw std::logic_error(err);
	}
	Event3< int > event = boost::any_cast< Event3< int > >(evt);
	if (event.value != 10)
	{
		std::string err = "my_locking_unexpected_event_handler: The event value is not 10: ";
		err += boost::lexical_cast< std::string >(event.value);
		throw std::logic_error(err);
	}

	if (state != typeid(InitialState))
	{
		std::string err = "my_locking_unexpected_event_handler: The current state is not InitialState: ";
		err += state.name();
		throw std::logic_error(err);
	}

	if (id != InitialState::state_id)
	{
		std::string err = "my_locking_unexpected_event_handler: The current state id is not InitialState::state_id (";
		const fsm::state_id_t state_id = InitialState::state_id;
		err += boost::lexical_cast< std::string >(state_id);
		err += "): ";
		err += boost::lexical_cast< std::string >(id);
		throw std::logic_error(err);
	}
}

BOOST_AUTO_TEST_CASE(locking_unexpected_events_handling)
{
	TEST_ENTER(locking_unexpected_events_handling);

	StateMachine_t fsm;
	TEST_REQUIRE(fsm.is_in_state< InitialState >());

	try
	{
		fsm.process(Event3< std::string >("oops"));
		TEST_REQUIRE(false);
	}
	catch(fsm::unexpected_event& e)
	{
		TEST_REQUIRE(e.what() != NULL);
		std::cout << "[unexpected_event::what(): " << e.what() << "]" << std::endl;
	}

	fsm.set_unexpected_event_handler(&my_locking_unexpected_event_handler);
	fsm.process(Event3< int >(10));
}

BOOST_AUTO_TEST_CASE(locking_bad_state_ids_handling)
{
	TEST_ENTER(locking_bad_state_ids_handling);

	StateMachine_t fsm;
	fsm.process(Event2());
	fsm.process(Event2());
	TEST_REQUIRE(fsm.is_in_state< FinalState >());

	try
	{
		// Lets try to switch to invalid state
		fsm.process(Event3< fsm::state_id_t >(100));
		TEST_REQUIRE(false);
	}
	catch(fsm::bad_state_id& e)
	{
		TEST_REQUIRE(e.what() != NULL);
		std::cout << "[bad_state_id::what(): " << e.what() << "]" << std::endl;
	}

	try
	{
		// Lets try to get type_info of invalid state
		fsm.get_state_type(fsm::state_id_t(100));
		TEST_REQUIRE(false);
	}
	catch (fsm::bad_state_id& e)
	{
		TEST_REQUIRE(e.what() != NULL);
	}

	try
	{
		// Lets try to get name of invalid state
		fsm.get_state_name(fsm::state_id_t(100));
		TEST_REQUIRE(false);
	}
	catch (fsm::bad_state_id& e)
	{
		TEST_REQUIRE(e.what() != NULL);
	}
}

BOOST_AUTO_TEST_CASE(locking_accessors)
{
	TEST_ENTER(locking_accessors);

	// get_state_name
	StateMachine_t fsm;
	std::string state_name1 = fsm.get_current_state_name();
	TEST_REQUIRE(state_name1 == "Initial state");

	std::string state_name2 = fsm.get_state_name(State2::state_id);
	TEST_REQUIRE(state_name2 == State2::get_state_name());

	// get_state
	TEST_REQUIRE(fsm.get_current_state_type() == typeid(InitialState));
	TEST_REQUIRE(fsm.get_state_type(State1::state_id) == typeid(State1));

	// get_state_id
	TEST_REQUIRE(fsm.get_current_state_id() == InitialState::state_id);
	fsm.process(Event1());
	TEST_REQUIRE(fsm.get_current_state_id() == State1::state_id);
}

BOOST_AUTO_TEST_CASE(locking_copying)
{
	TEST_ENTER(locking_copying);

	StateMachine_t fsm1;
	fsm1.process(Event1());
	TEST_REQUIRE(fsm1.is_in_state< State1 >());

	// We can construct a copy of the whole state machine. This will invoke all needed
	// copy constructors including states and virtual bases. The copy will be in the same state
	// as the original state machine.
	StateMachine_t fsm2 = fsm1;
	TEST_REQUIRE(fsm2.is_in_state< State1 >());
	TEST_REQUIRE(fsm2.get< InitialState >().m_Event1Received == fsm1.get< InitialState >().m_Event1Received);
	TEST_REQUIRE(fsm2.get< InitialState >().m_Event2Received == fsm1.get< InitialState >().m_Event2Received);
	TEST_REQUIRE(fsm2.get< InitialState >().m_EventsTrace == fsm1.get< InitialState >().m_EventsTrace);

	// Check that these state machines work independently now
	fsm1.process(Event1());
	fsm2.process(Event3< int >(10));
	TEST_REQUIRE(fsm1.is_in_state< FinalState >());
	TEST_REQUIRE(fsm1.get< State1 >().m_EventBaseReceived);
	TEST_REQUIRE(!fsm1.get< State1 >().m_Event3Received);
	TEST_REQUIRE(fsm2.is_in_state< State2 >());
	TEST_REQUIRE(fsm2.get< State1 >().m_Event3Received);
	TEST_REQUIRE(!fsm2.get< State1 >().m_EventBaseReceived);

	// Assignment works too, but beware. The C++ Standard does not specify how many times
	// the assignment operator will be called for virtual base classes. The state machine
	// implementation feels quite fine for that but some analogue of our CommonData could break.
	fsm2 = fsm1;
	TEST_REQUIRE(fsm1.is_in_state< FinalState >());
	TEST_REQUIRE(fsm2.is_in_state< FinalState >());
	TEST_REQUIRE(fsm2.get< InitialState >().m_Event1Received == fsm1.get< InitialState >().m_Event1Received);
	TEST_REQUIRE(fsm2.get< InitialState >().m_Event2Received == fsm1.get< InitialState >().m_Event2Received);
	TEST_REQUIRE(fsm2.get< State1 >().m_EventBaseReceived == fsm1.get< State1 >().m_EventBaseReceived);
	TEST_REQUIRE(fsm2.get< State1 >().m_Event3Received == fsm1.get< State1 >().m_Event3Received);
	TEST_REQUIRE(fsm2.get< InitialState >().m_EventsTrace == fsm1.get< InitialState >().m_EventsTrace);

	// An assignment of a similar state machine with different mutex type is also possible
/*
	typedef fsm::locking_state_machine< StatesList_t, void, void, boost::mutex > AnotherStateMachine_t;
	AnotherStateMachine_t another_fsm;
	another_fsm = fsm2;
	TEST_REQUIRE(another_fsm.is_in_state< FinalState >());
	TEST_REQUIRE(fsm2.get< InitialState >().m_Event1Received == another_fsm.get< InitialState >().m_Event1Received);
	TEST_REQUIRE(fsm2.get< InitialState >().m_Event2Received == another_fsm.get< InitialState >().m_Event2Received);
	TEST_REQUIRE(fsm2.get< State1 >().m_EventBaseReceived == another_fsm.get< State1 >().m_EventBaseReceived);
	TEST_REQUIRE(fsm2.get< State1 >().m_Event3Received == another_fsm.get< State1 >().m_Event3Received);
	TEST_REQUIRE(fsm2.get< InitialState >().m_EventsTrace == another_fsm.get< InitialState >().m_EventsTrace);
*/
}
