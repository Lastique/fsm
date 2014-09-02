/*!
* (C) 2006 Andrey Semashev
* 
* Use, modification and distribution is subject to the Boost Software License, Version 1.0.
* (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*
* \file   turnstile.cpp
* \author Andrey Semashev
* \date   08.01.2007
* 
* \brief  A tutorial example code
*/

#include <string>
#include <iostream>
#include <boost/mpl/vector.hpp>
#include <boost/fsm/state_machine.hpp>

// External API emulation
void CheckTicketValidity(std::string const& TicketID);
void RaiseUnauthorizedPassAlarm();
void UnlockTheTurnstile();
void LockTheTurnstile();

using namespace boost;

// Idle state. In this state the machine is waiting for a new request.
struct Idle;
// Processing state. Fee for passing through the turnstile is being taken.
struct Processing;
// Passing state. The passenger is allowed to pass through the turnstile.
struct Passing;

typedef mpl::vector< Idle, Processing, Passing >::type StateList;

// The event of a passenger trying to pass through the turnstile
struct PassengerPassing {};
// The event of a passenger have passed the turnstile
struct PassengerPassed {};
// The event of putting a ticket into the turnstile
struct Ticket
{
  // An event may contain data.
  // Ticket identification number
  std::string ID;

  // Constructor
  Ticket(std::string const& id) : ID(id) {}
};
// The event of ticket validity check result
struct ValidityCheckResult
{
  // The flag shows if a ticket is valid
  bool Valid;

  // Constructor
  ValidityCheckResult(bool valid) : Valid(valid) {}
};


// Idle state definition
struct Idle :
  public fsm::state< Idle, StateList >
{
  // In Idle state we are waiting for a ticket to process
  void on_process(Ticket const& ticket)
  {
    std::cout << "Ticket with ID: " << ticket.ID << ". Please wait..." << std::endl;
    // Let's assume this function initiates the ticket validity check
    // This may be an asynchronous operation (database query, for example), so we will have
    // to wait for a response
    CheckTicketValidity(ticket.ID);
    switch_to< Processing >();
  }

  // In case if a passenger tries to pass without a ticket we should alarm
  void on_process(PassengerPassing const&)
  {
    std::cout << "You may not pass. Please, put your ticket into the turnstile first." << std::endl;
    // This function raises alarm
    RaiseUnauthorizedPassAlarm();
  }
};

// Processing state definition
struct Processing :
  public fsm::state< Processing, StateList >
{
  // In this state we are waiting for ticket validity check is being processed
  void on_process(ValidityCheckResult const& result)
  {
    if (result.Valid)
    {
      // The passenger may pass
      std::cout << "You may pass" << std::endl;
      // Let's assume this function unlocks the turnstile
      UnlockTheTurnstile();
      // And after that the turnstile should be waiting for the passenger to pass
      switch_to< Passing >();
    }
    else
    {
      // The ticket is not valid
      std::cout << "Your ticket is not valid. Please, obtain another one." << std::endl;
      switch_to< Idle >();
    }
  }

  // The on_process method may be a template
  // This method will be called in case if some another event arrives
  template< typename T >
  void on_process(T const&)
  {
    std::cout << "Please wait, the ticket validity check is in process..." << std::endl;
  }
};

// Passing state definition
struct Passing :
  public fsm::state< Passing, StateList >
{
  // A state may contain data
  bool m_fPassengerPassing;

  Passing() : m_fPassengerPassing(false) {}

  // In this state we should allow a passenger to pass
  void on_process(PassengerPassing const&)
  {
    // Since we may allow to pass only one passenger, we shall use this flag
    // to detect if a second person tries to pass for free
    if (!m_fPassengerPassing)
    {
      // It's ok, the first passenger passes
      m_fPassengerPassing = true;
    }
    else
    {
      // Someone tries to pass for free
      std::cout << "Please wait until the first passenger passes" << std::endl;
      RaiseUnauthorizedPassAlarm();
    }
  }

  // When the passenger have passed we should lock the turnstile again
  void on_process(PassengerPassed const&)
  {
    m_fPassengerPassing = false;
    LockTheTurnstile();
    // And loop back to Idle state
    switch_to< Idle >();
  }
};

// State machine type definition
typedef fsm::state_machine< StateList > TurnstileStateMachine;


int main()
{
  TurnstileStateMachine turnstile;

  // A passenger comes to turnstile and puts his ticket into it.
  turnstile.process(Ticket("1234567"));
  // The device sends a request to validate the ticket and displays a message asking to wait a while
  // Now the validation response arrives, let's say it's positive
  turnstile.process(ValidityCheckResult(true));
  // The turnstile is unlocked now, the passenger passes
  turnstile.process(PassengerPassing());
  // And when he have passed the device gets locked again
  turnstile.process(PassengerPassed());

  return 0;
}


// The procedure emulates ticket validity check (a database request, for example)
void CheckTicketValidity(std::string const& TicketID)
{
  std::cout << "[Ticket validity check request]" << std::endl;
}

// The procedure raises alarm in case of pass for free
void RaiseUnauthorizedPassAlarm()
{
  std::cout << "\b[!!! Someone tries to pass for free !!!]" << std::endl;
}

// The procedure unlocks the turnstile
void UnlockTheTurnstile()
{
  std::cout << "[Turnstile is unlocked]" << std::endl;
}

// The procedure locks the turnstile
void LockTheTurnstile()
{
  std::cout << "[Turnstile is locked]" << std::endl;
}
