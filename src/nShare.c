/**
 * Implementation of a concurrent sharing system using low level synchronization tools.
 * 
 * @author Ignacio Slater Muñoz
 * @version 1.0b7
 * @since 1.0
 */
#include <stdarg.h>
#include "nSysimp.h"
#include <nSystem.h>
#include "fifoqueues.h"

char *message;
int pendingRequests = 0;
int isSharing = FALSE;
char *preamble = "DEBUG   ";

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
  nPrintf("%s[nRequest]   Pending requests: %d\n", preamble, pendingRequests);
  START_CRITICAL();
  nPrintf("%s[nRequest]   Entered critical section.\n", preamble);
  pendingRequests++;
  nPrintf("%s[nRequest]   Pending requests: %d\n",preamble, pendingRequests);
  PutTask(t->send_queue, current_task);
  nPrintf("%s[nRequest]   Added 0x%X to 0x%X's send queue\n",preamble, current_task, t);
  END_CRITICAL();
  nPrintf("%s[nRequest]   Exited critical section.\n", preamble);
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
  nPrintf("%s[nShare]   Entered critical section.\n", preamble);
  isSharing = TRUE;
  nPrintf("%s[nShare]   0x%X started sharing %X\n", preamble, current_task, data);
  nPrintf("%s[nShare]   Looking for requests\n", preamble);
  while (!EmptyQueue(current_task->send_queue))
  {
    nTask requestingTask = GetTask(current_task->send_queue);
    nPrintf("%s[nShare]   Answering request for task 0x%X\n", preamble, requestingTask);
  }
  isSharing = FALSE;
  nPrintf("%s[nShare]   0x%X finished sharing\n",preamble, current_task);
  END_CRITICAL();
  nPrintf("%s[nShare]   Exited critical section.\n", preamble);
}
