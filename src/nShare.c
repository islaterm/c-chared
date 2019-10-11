/**
 * Implementation of a concurrent sharing system using low level synchronization tools.
 * 
 * @author Ignacio Slater Muñoz
 * @version 1.0b9
 * @since 1.0
 */
#include <stdarg.h>
#include "nSysimp.h"
#include <nSystem.h>
#include "fifoqueues.h"

/** 
 * Possible status of a request or share task.
 * 
 * WAIT_SHARE
 *    the task is waiting indefinitely for another to call nShare
 * WAIT_SHARE_TIMEOUT
 *    the task is waiting for another one to call nShare with a timeout
*/
typedef enum shareStatus
{
  WAIT_SHARE,
  WAIT_SHARE_TIMEOUT
} ShareStatus;

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
  nPrintf("%s[nRequest]   Pending requests: %d\n", preamble, pendingRequests);
  START_CRITICAL();

  nPrintf("%s[nRequest]   Entered critical section.\n", preamble);
  pendingRequests++;
  nPrintf("%s[nRequest]   Pending requests: %d\n", preamble, pendingRequests);

  PutTask(t->send_queue, current_task);
  nPrintf("%s[nRequest]   0x%X's timeout: %d\n", preamble, current_task, timeout);
  if (timeout > 0)
  {
    current_task->status = WAIT_SHARE_TIMEOUT;
    ProgramTask(timeout);
    nPrintf("%s[nRequest]   0x%X started a request with timeout: %d\n", preamble,
            current_task, timeout);
  }
  else
  {
    current_task->status = WAIT_SHARE;
    nPrintf("%s[nRequest]   0x%X started a request without timeout\n", preamble,
            current_task);
  }
  ResumeNextReadyTask();
  nPrintf("%s[nRequest]   Added 0x%X to 0x%X's send queue\n", preamble, current_task, t);
  pendingRequests--;
  nPrintf("%s[nRequest]   Pending requests: %d\n", preamble, pendingRequests);

  END_CRITICAL();
  nPrintf("%s[nRequest]   Exited critical section.\n", preamble);
  nPrintf("%s[nRequest]   0x%X received the following answer: %X.\n", preamble,
          current_task, current_task->send.msg);
  return current_task->send.msg;
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
  nPrintf("%s[nShare]   0x%X started sharing %X\n", preamble, current_task, data);
  nPrintf("%s[nShare]   Looking for requests\n", preamble);
  while (!EmptyQueue(current_task->send_queue))
  {
    nTask requestingTask = GetTask(current_task->send_queue);
    if (requestingTask->status == WAIT_SHARE || requestingTask->status == WAIT_SHARE_TIMEOUT)
    {
      nPrintf("%s[nShare]   Answering request for task 0x%X\n", preamble, requestingTask);
      if (requestingTask->status == WAIT_SHARE_TIMEOUT)
      {
        CancelTask(requestingTask);
        nPrintf("%s[nShare]   Canceling task 0x%X because it reached it's timeout\n",
                preamble, requestingTask);
      }
      requestingTask->status = READY;
      PushTask(ready_queue, requestingTask);
      nPrintf("%s[nShare]   Added 0x%X to the ready queue\n", preamble, requestingTask);
    }
    requestingTask->send.msg = data;
  }
  nPrintf("%s[nShare]   0x%X finished sharing\n", preamble, current_task);
  END_CRITICAL();
  nPrintf("%s[nShare]   Exited critical section.\n", preamble);
}
