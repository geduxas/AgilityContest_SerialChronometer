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

/**
 * Initialize module:
 * - create internal data structures,
 * - verify configuration consistency
 * - copy configuration pointer
 * @param config pointer to configuration
 * @return 0:on success, -1:on error
 */
ADDAPI int ADDCALL module_init(configuration *config);

/**
 * De-initialize module
 * - close ( if yet-open ) device
 * - free internal data structures
 * @return 0:success -1:error
 */
ADDAPI int ADDCALL module_end();

/**
 * Open device, set speed an protocol. store handler
 * @return 0:success -1:error
 */
ADDAPI int ADDCALL module_open();

/**
 * Close device
 * Make it still available, but unable for I/O operations
 * @return 0:success -1:error
 */
ADDAPI int ADDCALL module_close();

/**
 * Perform blocking-read ( or timeout after x seconds )
 * @param buffer pointer to buffer for data received. On read timeout should contain "" ( empty string )
 * @param length length of receiving buffer
 * @return length of received data, 0 on timeout or -1 on error
 */
ADDAPI int ADDCALL module_read(char *buffer,size_t length);

/**
 * Send data from application to module in CommAPI format
 * @param tokens for the command to be sent
 * @param length number of tokens
 * @return 0:message sent to chronometer -1:error
 */
ADDAPI int ADDCALL module_write(char **tokens,size_t ntokens);

/**
 * Retrieve last error message string
 * @return pointer to last error. program won't free() received pointer
 */
ADDAPI char * ADDCALL module_error();

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif //SERIALCHRONOMETER_MODULES_H
