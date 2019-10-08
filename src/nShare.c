/**
 * Implementation of a concurrent sharing system using low level synchronization tools.
 * 
 * @author Ignacio Slater Muñoz
 * @version 1.0b4
 * @since 1.0
 */
#include <stdarg.h>
#include "nSysimp.h"
#include <nSystem.h>
#include "fifoqueues.h"

char *message;
FifoQueue pendingRequests;

void println();
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
  if (QueueLength(current_task->send_queue) == 0)
  {
    nPrintf("There are no pending requests, returning.\n");
    return;
  }
  // Coloca la tarea actual al final de la cola
  PushTask(ready_queue, current_task);
  nPrintf("Located share task at the end of the send queue");
  while (!EmptyQueue(current_task->send_queue))
  {
    nPrintf("nShare is waiting for pending requests to finish.\n");
    nTask waitingTask = GetTask(current_task->send_queue);
    waitingTask->status = READY;
    PutTask(ready_queue, waitingTask);
    nPrintf("Located task %s in the ready queue.\n");
  }
  nPrintf("All requests have been answered.\n"); 
  ResumeNextReadyTask();
  END_CRITICAL();
  nPrintf("Exited critical section from nShare\n");
}

