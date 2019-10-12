/**
 * Implementation of a concurrent sharing system using low level synchronization tools.
 * 
 * @author Ignacio Slater Muñoz
 * @version 1.0b10
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
  WAIT_SHARE_TIMEOUT,
  WAIT_RELEASE
} ShareStatus;

const char
    *DEBUG = "DEBUG   ",
    *ERROR = "ERROR   ";
    
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
  nPrintf("%s[nRequest]   Pending requests: %d\n", DEBUG, pendingRequests);
  START_CRITICAL();

  nPrintf("%s[nRequest]   Entered critical section.\n", DEBUG);
  pendingRequests++;
  nPrintf("%s[nRequest]   Pending requests: %d\n", DEBUG, pendingRequests);

  PutTask(t->send_queue, current_task);
  nPrintf("%s[nRequest]   0x%X's timeout: %d\n", DEBUG, current_task, timeout);
  if (timeout > 0)
  {
    current_task->status = WAIT_SHARE_TIMEOUT;
    ProgramTask(timeout);
    nPrintf("%s[nRequest]   0x%X started a request with timeout: %d\n", DEBUG,
            current_task, timeout);
  }
  else
  {
    current_task->status = WAIT_SHARE;
    nPrintf("%s[nRequest]   0x%X started a request without timeout\n", DEBUG,
            current_task);
  }
  ResumeNextReadyTask();
  nPrintf("%s[nRequest]   Added 0x%X to 0x%X's send queue\n", DEBUG, current_task, t);
  pendingRequests--;
  nPrintf("%s[nRequest]   Pending requests: %d\n", DEBUG, pendingRequests);

  END_CRITICAL();
  nPrintf("%s[nRequest]   Exited critical section.\n", DEBUG);
  nPrintf("%s[nRequest]   0x%X received the following answer: %X.\n", DEBUG,
          current_task, current_task->send.msg);
  return current_task->send.msg;
}

void nRelease(nTask t)
{
  const char *context = "[nRelease]   ";
  nPrintf("%s%sfunction not implemented\n", ERROR, context);
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
  char *context = "[nShare]     ";
  nPrintf("%s%sEntered critical section.\n", DEBUG, context);
  nPrintf("%s%s0x%X started sharing %X\n", DEBUG, context, current_task, data);
  nPrintf("%s%sLooking for requests\n", DEBUG, context);
  while (!EmptyQueue(current_task->send_queue))
  {
    nTask requestingTask = GetTask(current_task->send_queue);
    if (requestingTask->status == WAIT_SHARE || requestingTask->status == WAIT_SHARE_TIMEOUT)
    {
      current_task->status = nPrintf("%s%sAnswering request for task 0x%X\n", DEBUG,
                                     context, requestingTask);
      if (requestingTask->status == WAIT_SHARE_TIMEOUT)
      {
        CancelTask(requestingTask);
        nPrintf("%s%sCanceling task 0x%X because it reached it's timeout\n",
                DEBUG, context, requestingTask);
      }
      requestingTask->status = READY;
      requestingTask->send.msg = data;
      PushTask(ready_queue, requestingTask);
      nPrintf("%s%sAdded 0x%X to the ready queue\n", DEBUG, context, requestingTask);
    }
    current_task->status = WAIT_RELEASE;
    nPrintf("%s%sWaiting for all requests to release the data\n", DEBUG, context);
    ResumeNextReadyTask();
  }
  nPrintf("%s%s0x%X finished sharing\n", DEBUG, context, current_task);
  END_CRITICAL();
  nPrintf("%s%sExited critical section.\n", DEBUG, context);
}
