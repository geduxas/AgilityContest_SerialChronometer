//
// Created by jantonio on 20/06/19.
//

#ifndef SERIALCHRONOMETER_MODULES_H
#define SERIALCHRONOMETER_MODULES_H
/* add.h

   Declares the functions to be imported by our application, and exported by our
   DLL, in a flexible and elegant way.
*/
#ifdef _WIN32

/* You should define ADD_EXPORTS *only* when building the DLL. */
#ifdef ADD_EXPORTS
#define ADDAPI __declspec(dllexport)
#else
#define ADDAPI __declspec(dllimport)
#endif

/* Define calling convention in one place, for convenience. */
#define ADDCALL __cdecl

#else /* _WIN32 not defined. */

/* Define with no value on non-Windows OSes. */
  #define ADDAPI
  #define ADDCALL

#endif

#include "sc_config.h"
/* Make sure functions are exported with C linkage under C++ compilers. */

#ifdef __cplusplus
extern "C"
{
#endif

/* Declare our Add function using the above definitions. */
ADDAPI int ADDCALL module_init(configuration *config);
ADDAPI int ADDCALL module_end();
ADDAPI int ADDCALL module_open();
ADDAPI int ADDCALL module_close();
ADDAPI int ADDCALL module_read(char *buffer,size_t length);
ADDAPI int ADDCALL module_write(char *buffer,size_t length);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif //SERIALCHRONOMETER_MODULES_H
