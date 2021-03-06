/*!
 * (C) 2006 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   event.hpp
 * \author Andrey Semashev
 * \date   23.12.2006
 * 
 * \brief  This header is the Boost.FSM library implementation, see the library documentation
 *         at http://www.boost.org/libs/state_machine/doc/state_machine.html. In this file
 *         simplified events generation support is implemented.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1020)
#pragma once
#endif // _MSC_VER > 1020

#ifndef BOOST_FSM_EVENT_HPP_INCLUDED_
#define BOOST_FSM_EVENT_HPP_INCLUDED_

#include <boost/ref.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/int.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/fsm/detail/prologue.hpp>

#ifndef BOOST_FSM_MAX_EVENT_ARGS
#define BOOST_FSM_MAX_EVENT_ARGS 10
#endif // BOOST_FSM_MAX_EVENT_ARGS

namespace boost {

namespace fsm {

namespace aux {

    /*!
    *    \brief An internal structure for mapping fsm::event template parameters to tuple's ones
    *
    *    To shorten names generated by compiler that involve fsm::event its default
    *    template parameter types are void. This structure is used to map these void types
    *    into tuples::null_type to pass to tuple template.
    */
    template< typename T >
    struct tuple_arg_type :
        public mpl::identity< T >
    {
    };
    //! An internal structure for mapping fsm::event template parameters to tuple's ones (specialization for void)
    template< >
    struct tuple_arg_type< void > :
        public mpl::identity< tuples::null_type >
    {
    };

} // namespace aux

#define BOOST_FSM_TUPLE_ARG_TYPE(z, iter, data)\
    typename aux::tuple_arg_type< BOOST_PP_CAT(data, iter) >::type

//! Tagged event type
template< typename TagT, BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(BOOST_FSM_MAX_EVENT_ARGS, typename T, void) >
struct event :
    public tuple< BOOST_PP_ENUM(BOOST_FSM_MAX_EVENT_ARGS, BOOST_FSM_TUPLE_ARG_TYPE, T) >
{
    //! Tag type
    typedef TagT tag_type;
    //! Tuple type
    typedef tuple< BOOST_PP_ENUM(BOOST_FSM_MAX_EVENT_ARGS, BOOST_FSM_TUPLE_ARG_TYPE, T) > tuple_type;

    //! Default constructor
    event() {}
    //! Constructor from a tuple
    explicit event(tuple_type const& that) : tuple_type(that) {}

    //! Assignment operator
    event& operator= (event const& that)
    {
        tuple_type::operator= (static_cast< tuple_type const& >(that));
        return *this;
    }
    //! Assignment operator
    template< typename U >
    event& operator= (U const& that)
    {
        tuple_type::operator= (that);
        return *this;
    }

    //! An accessor to tuple
    tuple_type& get_tuple() { return *this; }
    //! An accessor to tuple
    tuple_type const& get_tuple() const { return *this; }
};

#undef BOOST_FSM_TUPLE_ARG_TYPE

//! Integral constant-tagged event type
template< int TagV, BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(BOOST_FSM_MAX_EVENT_ARGS, typename T, void) >
struct event_c :
    public event< mpl::int_< TagV >, BOOST_PP_ENUM_PARAMS(BOOST_FSM_MAX_EVENT_ARGS, T) >
{
private:
    //! Base type
    typedef event< mpl::int_< TagV >, BOOST_PP_ENUM_PARAMS(BOOST_FSM_MAX_EVENT_ARGS, T) > base_type;

public:
    //! Tuple type import
    typedef typename base_type::tuple_type tuple_type;

public:
    //! Default constructor
    event_c() {}
    //! Constructor from a tuple
    explicit event_c(tuple_type const& that) : base_type(that) {}

    //! Assignment operator
    event_c& operator= (event_c const& that)
    {
        base_type::operator= (static_cast< base_type const& >(that));
        return *this;
    }
    //! Assignment operator
    template< typename U >
    event_c& operator= (U const& that)
    {
        base_type::operator= (that);
        return *this;
    }
};

namespace aux {

    /*!
    *    \brief An internal mapper that transforms reference_wrapper into a real reference type
    *           to ease fsm::event usage in the on_process handlers and transition rules
    */
    template< typename T >
    struct event_arg_type :
        public mpl::if_<
            is_reference_wrapper< T >,
            add_reference< typename unwrap_reference< T >::type >,
            mpl::identity< T >
        >::type
    {
    };

#define BOOST_FSM_EVENT_ARG_TYPE(z, iter, data)\
    typename event_arg_type< BOOST_PP_CAT(data, iter) >::type

    //! An internal fsm::event type maker
    template< typename TagT, BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(BOOST_FSM_MAX_EVENT_ARGS, typename T, void) >
    struct event_type_maker
    {
        typedef event< TagT, BOOST_PP_ENUM(BOOST_FSM_MAX_EVENT_ARGS, BOOST_FSM_EVENT_ARG_TYPE, T) > type;
    };

    //! An internal fsm::event type maker
    template< int TagV, BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(BOOST_FSM_MAX_EVENT_ARGS, typename T, void) >
    struct event_c_type_maker
    {
        typedef event_c< TagV, BOOST_PP_ENUM(BOOST_FSM_MAX_EVENT_ARGS, BOOST_FSM_EVENT_ARG_TYPE, T) > type;
    };

#undef BOOST_FSM_EVENT_ARG_TYPE

} // namespace aux

#define BOOST_FSM_MAKE_EVENT(z, iter, data)\
    template< typename TagT BOOST_PP_ENUM_TRAILING_PARAMS(iter, typename T) >\
    BOOST_FSM_FORCEINLINE typename aux::event_type_maker< TagT BOOST_PP_ENUM_TRAILING_PARAMS(iter, T) >::type\
    make_event(BOOST_PP_ENUM_BINARY_PARAMS(iter, T, const& Arg))\
    {\
        typedef typename aux::event_type_maker< TagT BOOST_PP_ENUM_TRAILING_PARAMS(iter, T) >::type event_type;\
        return event_type(make_tuple(BOOST_PP_ENUM_PARAMS(iter, Arg)));\
    }\
    template< int TagV BOOST_PP_ENUM_TRAILING_PARAMS(iter, typename T) >\
    BOOST_FSM_FORCEINLINE typename aux::event_c_type_maker< TagV BOOST_PP_ENUM_TRAILING_PARAMS(iter, T) >::type\
    make_event(BOOST_PP_ENUM_BINARY_PARAMS(iter, T, const& Arg))\
    {\
        typedef typename aux::event_c_type_maker< TagV BOOST_PP_ENUM_TRAILING_PARAMS(iter, T) >::type event_type;\
        return event_type(make_tuple(BOOST_PP_ENUM_PARAMS(iter, Arg)));\
    }

//  make_event functions implementation
BOOST_PP_REPEAT(BOOST_FSM_MAX_EVENT_ARGS, BOOST_FSM_MAKE_EVENT, ~)

#undef BOOST_FSM_MAKE_EVENT

} // namespace fsm

} // namespace boost

#endif // BOOST_FSM_EVENT_HPP_INCLUDED_
