/*!
 * (C) 2006 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   exceptions.hpp
 * \author Andrey Semashev
 * \date   24.11.2006
 * 
 * \brief  This header is the Boost.FSM library implementation, see the library documentation
 *         at http://www.boost.org/libs/state_machine/doc/state_machine.html. In this file
 *         exception classes are implemented.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1020)
#pragma once
#endif // _MSC_VER > 1020

#ifndef BOOST_FSM_EXCEPTIONS_HPP_INCLUDED_
#define BOOST_FSM_EXCEPTIONS_HPP_INCLUDED_

#ifdef __GNUC__
#include <cxxabi.h>
#include <memory.h>
#endif // __GNUC__
#include <exception>
#include <string>
#include <sstream>
#include <typeinfo>
#include <boost/any.hpp>
#include <boost/optional.hpp>

// Some compilers don't have type_info in std namespace
#if defined(__INTEL_COMPILER) && defined(_MSC_VER) && (_MSC_VER <= 1310)

namespace std {
    using ::type_info;
} // namespace std

#endif // defined(__INTEL_COMPILER) && defined(_MSC_VER) && (_MSC_VER <= 1310)

namespace boost {

namespace fsm {

//! State identifiers type
typedef unsigned int state_id_t;

namespace aux {

#ifdef __GNUC__

    //! A simple scope guard for automatic memory free
    struct auto_free
    {
        auto_free(void* p) : p_(p) {}
        ~auto_free() { free(p_); }

    private:    
        void* p_;
    };

#endif // __GNUC__

    //! The function constructs a string, describing the type which type info is passed as an argument (implementation)
    template< typename TypeInfoT >
    std::string construct_type_name_impl(TypeInfoT const& info)
    {
        const char* const pFullName = info.name();

#ifdef __GNUC__

        // GCC returns decorated type name, will need to demangle it using ABI
        int DemanglingStatus;
        size_t size = 0;
        char* pUndecorated = abi::__cxa_demangle(pFullName, NULL, &size, &DemanglingStatus);
        auto_free guard(pUndecorated);

        if (pUndecorated != NULL)
            return pUndecorated;
        else
            return pFullName;

#elif defined(_MSC_VER)

        // VC and ICL return the undecorated type names but add keywords
        // struct, class, union and enum in the notation. These keywords have to be removed.
        std::string result = pFullName;

        std::string::size_type pos;
        while ((pos = result.find("struct ")) != std::string::npos)
            result.erase(pos, sizeof("struct ") - 1);
        while ((pos = result.find("class ")) != std::string::npos)
            result.erase(pos, sizeof("class ") - 1);
        while ((pos = result.find("union ")) != std::string::npos)
            result.erase(pos, sizeof("union ") - 1);
        while ((pos = result.find("enum ")) != std::string::npos)
            result.erase(pos, sizeof("enum ") - 1);

        return result;

#else

        // As for other compilers we don't know the format of pFullName and return it as is
        return pFullName;

#endif
    }

    //! The function constructs a string, describing the type which type info is passed as an argument
    inline std::string construct_type_name(std::type_info const& info)
    {
        return construct_type_name_impl(info);
    }

} // namespace aux

//! Base class for all exception types thrown by the library
class BOOST_FSM_EXTERNALLY_VISIBLE fsm_error :
    public std::exception
{
private:
    //! Type info of the state in which the state machine persisted when the error occurred
    std::type_info const& m_StateType;
    //! Identifier of the state in which the state machine persisted when the error occurred
    state_id_t m_StateID;
    //! Name of the state in which the state machine persisted when the error occurred
    mutable optional< std::string > m_StateName;
    //! Error description. It only constructed when the exception is asked for.
    mutable optional< std::string > m_ErrorInfo;

public:
    //! Basic form of constructor
    fsm_error(std::type_info const& StateType, state_id_t StateID)
        : m_StateType(StateType), m_StateID(StateID)
    {
    }
    //! The constructor with state name provision
    fsm_error(std::string const& StateName, std::type_info const& StateType, state_id_t StateID)
        : m_StateType(StateType), m_StateID(StateID), m_StateName(StateName)
    {
    }
    //! Non-throwing destructor
    ~fsm_error() throw() {}

    //! An accessor to type info of the state in which the state machine persisted when the error occurred
    std::type_info const& current_state_type() const { return m_StateType; }
    //! An accessor to identifier of the state in which the state machine persisted when the error occurred
    state_id_t current_state_id() const { return m_StateID; }

    //! The method returns error description
    const char* what() const throw()
    {
        const char* pErrorInfo = "fsm_error";

        try
        {
            if (!!m_ErrorInfo)
                pErrorInfo = m_ErrorInfo->c_str();
        }
        catch (std::exception&)
        {
        }

        return pErrorInfo;
    }

protected:
    //! An accessor to error description string for derived classes
    optional< std::string >& error_info() const { return m_ErrorInfo; }
    //! An accessor to state name string for derived classes
    optional< std::string >& state_name() const { return m_StateName; }

private:
    //! The class is not assignable
    fsm_error& operator= (fsm_error const&);
};

//! An exception class thrown by library in case of an attempt to use invalid state identifier
class BOOST_FSM_EXTERNALLY_VISIBLE bad_state_id :
    public fsm_error
{
private:
    //! Invalid state identifier
    state_id_t m_BadStateID;

public:
    //! Basic version of constructor
    bad_state_id(state_id_t BadStateID, std::type_info const& StateType, state_id_t StateID)
        : fsm_error(StateType, StateID), m_BadStateID(BadStateID)
    {
    }
    //! A constructor with state name provision
    bad_state_id(state_id_t BadStateID, std::string const& StateName, std::type_info const& StateType, state_id_t StateID)
        : fsm_error(StateName, StateType, StateID), m_BadStateID(BadStateID)
    {
    }
    //! Non-throwing destructor
    ~bad_state_id() throw() {}

    //! An accessor to the invalid state identifier
    state_id_t state_id() const { return m_BadStateID; }

    //! The method returns error description
    const char* what() const throw()
    {
        const char* pErrorInfo = "bad_state_id: an attempt to use invalid state id detected";

        try
        {
            if (!error_info())
            {
                if (!state_name())
                {
                    // If no state name was provided on construction we shall construct one based on the state's type info
                    state_name() = aux::construct_type_name(current_state_type());
                }

                // Construct error description string
                std::ostringstream strm;
                strm << "bad_state_id: an attempt to use invalid state id "
                    << m_BadStateID << " detected in state '" << state_name().get() << "'";

                error_info() = strm.str();
            }
            pErrorInfo = error_info()->c_str();
        }
        catch (std::exception&)
        {
        }

        return pErrorInfo;
    }
};

//! An exception class thrown by library in case of an unexpected event detection
class BOOST_FSM_EXTERNALLY_VISIBLE unexpected_event :
    public fsm_error
{
private:
    //! The unexpected event
    any m_Event;

public:
    //! Basic version of constructor
    unexpected_event(any const& Event, std::type_info const& StateType, state_id_t StateID)
        : fsm_error(StateType, StateID), m_Event(Event)
    {
    }
    //! A constructor with state name provision
    unexpected_event(any const& Event, std::string const& StateName, std::type_info const& StateType, state_id_t StateID)
        : fsm_error(StateName, StateType, StateID), m_Event(Event)
    {
    }
    //! Non-throwing destructor
    ~unexpected_event() throw() {}

    //! An accessor to the unexpected event
    any const& event() const { return m_Event; }

    //! The method returns error description
    const char* what() const throw()
    {
        const char* pErrorInfo = "unexpected_event: the state machine does not expect the event in the current state";

        try
        {
            if (!error_info())
            {
                if (!state_name())
                {
                    // If no state name was provided on construction we shall construct one based on the state's type info
                    state_name() = aux::construct_type_name(current_state_type());
                }

                // Construct error description string
                std::ostringstream strm;
                strm << "unexpected_event: the state machine does not expect the event of type '"
                    << aux::construct_type_name(m_Event.type()) << "' in state '" << state_name().get() << "'";

                error_info() = strm.str();
            }
            pErrorInfo = error_info()->c_str();
        }
        catch (std::exception&)
        {
        }

        return pErrorInfo;
    }
};

} // namespace fsm

} // namespace boost

#endif // BOOST_FSM_EXCEPTIONS_HPP_INCLUDED_
