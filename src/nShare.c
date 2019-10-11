/**
 * Implementation of a concurrent sharing system using low level synchronization tools.
 * 
 * @author Ignacio Slater Muñoz
 * @version 1.0b6
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
 * Shares data.
 * When this function is called all the processes that made requests are unlocked.
 * 
 * @param data
 *    los datos que serán compartidos
*/
void nShare(char *data)
{
  START_CRITICAL();
  nPrintf("Entered critical section from nShare\n");
  isSharing = TRUE;
  nPrintf("0x%X started sharing %s\n", current_task, data);
  nPrintf("Looking for requests\n");
  while (!EmptyQueue(current_task->send_queue))
  {
    nTask *requestingTask = GetTask(current_task->send_queue);
    nPrintf("Answering request for task 0x%X\n");
  }
  isSharing = FALSE;
  nPrintf("0x%X finished sharing\n", current_task);
  END_CRITICAL();
  nPrintf("Exited critical section from nShare\n");
}
