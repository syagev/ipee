#pragma once

//the following macros define the minimum required platform.  The minimum required platform
//is the earliest version of Windows, Internet Explorer etc. that has the necessary features
//to run your application. the macros work by enabling all features available on platform 
//versions up to and including the version specified.

//MSDN has the latest info on corresponding values for different platforms.


#ifndef WINVER			//specifies that the minimum required platform is Windows Vista.
#define WINVER 0x0600
#endif

#ifndef _WIN32_WINNT    //specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0600
#endif

#ifndef _WIN32_WINDOWS  //specifies that the minimum required platform is Windows 98.
#define _WIN32_WINDOWS 0x0410
#endif

#ifndef _WIN32_IE	    //specifies that the minimum required platform is IE 7.0.
#define _WIN32_IE 0x0700
#endif
