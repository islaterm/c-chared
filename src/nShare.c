/**
 * Implementation of a concurrent sharing system using low level synchronization tools.
 * 
 * @author Ignacio Slater Muñoz
 * @version 1.0b15
 * @since 1.0
 */
#include <stdarg.h>
#include "nSysimp.h"
#include <nSystem.h>

#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#pragma region : CONSTANTS
static const char
    *DEBUG = "DEBUG   ",
    *ERROR = "ERROR   ";
#pragma endregion

#pragma region : LOCAL VARIABLES
static int
    nShareCounter = 0,   // Id for the current share task
    nReleaseCounter = 0, // Id for the current release task
    nRequestCounter = 0; // Id for the current request task
#pragma endregion

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
  const char *context = "[nRequest]   ";
  START_CRITICAL();

  nSetTaskName("REQUEST %d", nRequestCounter++);
  (t->pendingRequests)++;

  if (t->status == WAIT_REPLY)
  {
    nCurrentTask()->send.msg = t->send.msg;
    nPrintf("%s%s%s was active, %s returned %X\n", DEBUG, context, t->taskname,
            nGetTaskName(), nCurrentTask()->send.msg);
  }
  else
  {
    PushObj(t->requestQueue, nCurrentTask());
    nPrintf("%s%sAdded %s to %s's send queue\n", DEBUG, context, nGetTaskName(),
            t->taskname);
    if (timeout > 0)
    {
      nCurrentTask()->status = WAIT_SEND_TIMEOUT;
      ProgramTask(timeout);

      nPrintf("%s%s%s started a request with timeout: %d\n", DEBUG, context, nGetTaskName(),
              timeout);
    }
    else
    {
      nCurrentTask()->status = WAIT_SEND;
      nPrintf("%s%s%s started a request without timeout\n", DEBUG, context,
              nGetTaskName());
    }
    ResumeNextReadyTask();
  }

  if (t->status != WAIT_REPLY)
  {
    nPrintf("%s%s%s was READY, %s returns no answer.\n", DEBUG, context, t->taskname,
            nGetTaskName());
    return NULL;
  }

  END_CRITICAL();
  nPrintf("%s%s%s received the following answer: %X.\n", DEBUG, context, nGetTaskName(),
          (char *)nCurrentTask()->send.msg);
  t->send.msg = nCurrentTask()->send.msg;
  return (char *)nCurrentTask()->send.msg;
}

/** 
 * Notifies that the current task finished using the data.
 * 
 * @param t
 *    the task to be notified
*/
void nRelease(nTask t)
{
  const char *context = "[nRelease]   ";

  START_CRITICAL();
  nSetTaskName("RELEASE %d", nReleaseCounter++);

  if (t->status != WAIT_REPLY)
  {
    nPrintf("%s%s%s is not waiting for a release.\n", ERROR, context, t->taskname);
  }
  PushTask(ready_queue, nCurrentTask());
  nPrintf("%s%sAdded %s to the ready queue\n", DEBUG, context, nGetTaskName());

  if (--t->pendingRequests == 0)
  {
    t->status = READY;
    nPrintf("%s%sAdded %s to the ready queue\n", DEBUG, context, t->taskname);
    PushTask(ready_queue, t);
  }
  nPrintf("%s%s%s has %d pending requests\n", DEBUG, context, t->taskname,
          t->pendingRequests);

  ResumeNextReadyTask();

  END_CRITICAL();
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
  nSetTaskName("SHARE %d", nShareCounter++);

  char *context = "[nShare]     ";
  nPrintf("%s%s%s started sharing %X\n", DEBUG, context, nGetTaskName(), data);
  nPrintf("%s%sLooking for requests\n", DEBUG, context);
  while (!EmptyFifoQueue(nCurrentTask()->requestQueue))
  {
    nTask requestingTask = GetObj(nCurrentTask()->requestQueue);
    if (requestingTask->status == WAIT_SEND || requestingTask->status == WAIT_SEND_TIMEOUT)
    {
      if (requestingTask->status == WAIT_SEND_TIMEOUT)
      {
        CancelTask(requestingTask);
        nPrintf("%s%sCancelling task %s because it was answered before it's timeout\n",
                DEBUG, context, requestingTask->taskname);
      }
      requestingTask->status = READY;
      requestingTask->send.msg = data;
      PushTask(ready_queue, requestingTask);
      nPrintf("%s%sAdded %s to the ready queue\n", DEBUG, context,
              requestingTask->taskname);
    }
  }
  while (nCurrentTask()->pendingRequests)
  {
    nCurrentTask()->status = WAIT_REPLY;
    nPrintf("%s%sWaiting for all requests to release the data\n", DEBUG, context);
    ResumeNextReadyTask();
  }
  nPrintf("%s%s%s finished sharing\n", DEBUG, context, nGetTaskName());
  END_CRITICAL();
}
