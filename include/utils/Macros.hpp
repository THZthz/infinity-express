#ifndef IE_MACROS_HPP
#define IE_MACROS_HPP

// define _BUILD_DLL or _USE_AS_DLL to use API macro
#ifndef API
#	if defined(_WIN32) && defined(_BUILD_DLL)
// Building as a DLL
#		define API __declspec(dllexport)
#	elif defined(_WIN32) && defined(_USE_AS_DLL)
// Using as a DLL
#		define API __declspec(dllimport)
#	elif defined(__GNUC__) && defined(_BUILD_DLL)
// Building as a shared library
#		define API __attribute__((visibility("default")))
#	else
#		define API
#	endif
#endif

#endif // IE_MACROS_HPP

