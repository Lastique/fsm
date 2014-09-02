/*!
* (C) 2006 Andrey Semashev
*
* \file   stdafx.hpp
* \author Andrey Semashev
* \date   11.04.2006
*
* \brief  Basic headers inclusion
*/

#if defined(_MSC_VER) && _MSC_VER > 1000
#pragma once
#endif

#ifndef __STDAFX_H__
#define __STDAFX_H__

#if defined(_MSC_VER) && (_MSC_VER > 1310) && !defined(_CRT_SECURE_NO_DEPRECATE)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#define _WIN32_WINNT 0x0600
//#define BOOST_USE_WINDOWS_H

//#define BOOST_LWCO_NO_EXPLICIT_MEMORY_BARRIERS 1

#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/fsm/state_machine.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/lexical_cast.hpp>

namespace fsm = boost::fsm;

#endif // __STDAFX_H__
