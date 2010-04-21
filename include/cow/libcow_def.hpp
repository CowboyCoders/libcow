/*
Copyright 2010 CowboyCoders. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY COWBOYCODERS ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COWBOYCODERS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of CowboyCoders.
*/

#ifndef ___libcow_def___
#define ___libcow_def___

//
// Makes a leaner build
//
#ifdef WIN32
#   define WIN32_LEAN_AND_MEAN 
#   define WIN32_EXTRA_LEAN
#   define VC_EXTRALEAN 
#   include <Windows.h>
#endif

#include <boost/function.hpp>

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cassert>
#include <fstream>
#include <exception>
#include <list>

#include "cow/libcow_types.hpp"

//
// Project wide definitions for libcow
//

#ifdef WIN32
    // disable the "non dll-interface class A used as base 
    // for dll-interface class B" warnings
    #pragma warning(disable:4275)
    // disable the "class A needs to have dll-interface to
    // be used by clients of class B" warnings
    #pragma warning(disable:4251)
    // disable sprintf warnings (sprintf is used in boost.log)
    #pragma warning(disable:4996)
#endif


// dllexport-madness for Win32
#ifdef WIN32
    #ifdef cow_EXPORTS
        #define LIBCOW_EXPORT __declspec(dllexport)
    #else
        #define LIBCOW_EXPORT __declspec(dllimport)
    #endif
#else
    #define LIBCOW_EXPORT
#endif

//portable sleep
/*
#ifdef WIN32
    #define LIBCOW_SLEEP(n) Sleep((n)*1000) 
#else
    #define LIBCOW_SLEEP(n) sleep((n))
#endif
*/

#endif // ___libcow_def___