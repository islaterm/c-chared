/**
 * Implementation of a concurrent sharing system using low level synchronization tools.
 * 
 * @author Ignacio Slater Muñoz
 * @version 1.0b2
 * @since 1.0
 */
#include "nSysimp.h"
#include <nSystem.h>
#include <stdarg.h>

char *message;

char *nRequest(nTask t, int timeout)
{
  nFatalError("nRequest", "%s\n", "Not implemented");
  return "";
}

void nRelease(nTask t)
{
  nFatalError("nRelease not implemented", "%s");
}

/**
 * Ofrece datos para compartir.
 * Cuando se llama a esta función se desbloquean todos los procesos que esperaban datos.
 * 
 * 
 * @param data
 *    los datos que serán compartidos
*/
void nShare(char *data)
{
  START_CRITICAL();
  nPrintf("Entered critical section from nShare\n");
  END_CRITICAL();
  nPrintf("Exited critical section from nShare\n");
}