/**
 * Implementation of a concurrent sharing system using low level synchronization tools.
 * 
 * @author Ignacio Slater Muñoz
 * @version 1.0b5
 * @since 1.0
 */
#include <stdarg.h>
#include "nSysimp.h"
#include <nSystem.h>
#include "fifoqueues.h"

char *message;
int pendingRequests = 0;
int isSharing = FALSE;

/**
 * Requests data from a task.
 * If there is an active share task, then the request returns its answer, otherwise it
 * waits until a task shares information or for a certain amount of time elapses.
 * 
 * @param t
 *    the task that will share information.
 * @param timeout
 *    the time the request waits for a respo nse.
*/
char *nRequest(nTask t, int timeout)
{
  // char *data = NULL;
  nPrintf("Pending requests: %d\n", pendingRequests);
  START_CRITICAL();
  nPrintf("Entered nRequest critical section.\n");
  pendingRequests++;
  nPrintf("Pending requests: %d\n", pendingRequests);
  
  END_CRITICAL();
  nPrintf("Exited nRequest critical section.\n");
  return t->send.msg;
}

void nRelease(nTask t)
{
  nFatalError("nRelease", "%s", "not implemented\n");
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
  while (!EmptyQueue(current_task->send_queue))
  {
    /* code */
  }
  
  END_CRITICAL();
  nPrintf("Exited critical section from nShare\n");
}
