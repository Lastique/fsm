/*!
* (C) 2006 Andrey Semashev
* 
* \file   boost_testing_helpers.hpp
* \author Andrey Semashev
* \date   11.04.2006
* 
* \brief  Some auxiliary tools for Boost.Test
*/

#ifndef __BOOST_TESTSING_HELPERS_H__
#define __BOOST_TESTSING_HELPERS_H__

#include <boost/test/auto_unit_test.hpp>

//! A guarding object that prints a unit test result
struct _TestHlprResultGuard
{
	//! Test failure flag
	bool fFailed;
	//! Test name
	const char* pTestName;

	//! Constructor
	_TestHlprResultGuard(const char* pTestName_) : fFailed(false), pTestName(pTestName_) {}
	//! Destructor
	~_TestHlprResultGuard() {
		std::cout << pTestName << ": " << (fFailed ? "FAIL" : "SUCCESS") << std::endl;
	}
};

#define TEST_ENTER(test_name) _TestHlprResultGuard __test_guard(#test_name)

#define TEST_WARN( P ) \
	if (true) {\
		bool __test_result = (P);\
		if (!__test_result)\
			__test_guard.fFailed = true;\
		BOOST_CHECK_IMPL( __test_result, BOOST_TEST_STRINGIZE( P ), WARN, CHECK_PRED );\
	} else ((void)0)

#define TEST_CHECK( P ) \
	if (true) {\
		bool __test_result = (P);\
		if (!__test_result)\
			__test_guard.fFailed = true;\
		BOOST_CHECK_IMPL( __test_result, BOOST_TEST_STRINGIZE( P ), CHECK, CHECK_PRED );\
	} else ((void)0)

#define TEST_REQUIRE( P ) \
	if (true) {\
		bool __test_result = (P);\
		if (!__test_result)\
			__test_guard.fFailed = true;\
		BOOST_CHECK_IMPL( __test_result, BOOST_TEST_STRINGIZE( P ), REQUIRE, CHECK_PRED );\
	} else ((void)0)

#endif // __BOOST_TESTSING_HELPERS_H__
