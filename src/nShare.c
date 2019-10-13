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

#pragma region : CONSTANTS
static const char
    *DEBUG = "DEBUG   ",
    *ERROR = "ERROR   ";
#pragma endregion

#pragma region : LOCAL VARIABLES
static size_t pendingRequests = 0; // Number of requests to be answered
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
  nPrintf("%s%sEntered critical section.\n", DEBUG, context);

  nSetTaskName("REQUEST %d", nRequestCounter++);
  pendingRequests++;

  nPrintf("%s%sPending requests: %d\n", DEBUG, context, pendingRequests);

  if (t->status == WAIT_REPLY)
  {
    nCurrentTask()->send.msg = t->send.msg;
    nPrintf("%s%s%s was active, %s returned %X\n", DEBUG, context, t->taskname,
            nGetTaskName(), nCurrentTask()->send.msg);
    // PushTask(ready_queue, nCurrentTask());
    // nPrintf("%s%sAdded %s to the ready queue\n", DEBUG, context, nGetTaskName());
  }
  else
  {
    nPrintf("%s%s%s's timeout: %d\n", DEBUG, context, nGetTaskName(), timeout);
    if (timeout > 0)
    {
      nCurrentTask()->status = WAIT_SEND_TIMEOUT;
      ProgramTask(timeout);
      nPrintf("%s%s%s started a request with timeout: %d\n", DEBUG, context, nGetTaskName(),
              timeout);    }
    else
    {
      nCurrentTask()->status = WAIT_SEND;
      nPrintf("%s%s%s started a request without timeout\n", DEBUG, context,
              nGetTaskName());
      nPrintf("%s%s%s's status: %d\n", DEBUG, context, nGetTaskName(),
              nCurrentTask()->status);
    }
    PushObj(t->requestQueue, nCurrentTask());
    nPrintf("%s%sAdded %s to %s's send queue\n", DEBUG, context, nGetTaskName(),
            t->taskname);
    // nPrintf("%s%s%s's status: %d\n", DEBUG, context, nGetTaskName(),
    //         nCurrentTask()->status);
    // nPrintf("%s%sNext task: %s\n", DEBUG, context, nGetTaskName());

    ResumeNextReadyTask();
  }

  END_CRITICAL();
  nPrintf("%s%sExited critical section.\n", DEBUG, context);
  nPrintf("%s%s%s received the following answer: %X.\n", DEBUG, context, nGetTaskName(),
          (char *)nCurrentTask()->send.msg);
  t->send.msg = nCurrentTask()->send.msg;
  nPrintf("%s%sShare task: %s   Data: %X.\n\n", DEBUG, context, t->taskname, t->send.msg);
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
  START_CRITICAL();
  nPrintf("\n");
  nSetTaskName("RELEASE %d", nReleaseCounter++);
  const char *context = "[nRelease]   ";
  nPrintf("\n%s%sEntered critical section.\n", DEBUG, context);

  if (t->status != WAIT_REPLY)
  {
    nPrintf("%s%s%s is not waiting for a release.\n", ERROR, context, t->taskname);
  }
  PushTask(ready_queue, nCurrentTask());
  nPrintf("%s%sAdded %s to the ready queue\n", DEBUG, context, nGetTaskName());

  if (--pendingRequests == 0)
  {
    t->status = READY;
    nPrintf("%s%sAdded %s to the ready queue\n", DEBUG, context, t->taskname);
    PushTask(ready_queue, t);
  }
  nPrintf("%s%sPending requests: %d\n", DEBUG, context, pendingRequests);

  ResumeNextReadyTask();

  END_CRITICAL();
  nPrintf("%s%sExited critical section.\n\n", DEBUG, context);
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
  nPrintf("\n");
  nSetTaskName("SHARE %d", nShareCounter++);

  char *context = "[nShare]     ";
  nPrintf("\n%s%sEntered critical section.\n", DEBUG, context);
  nPrintf("%s%s%s started sharing %X\n", DEBUG, context, nGetTaskName(), data);
  nPrintf("%s%sLooking for requests\n", DEBUG, context);
  while (!EmptyFifoQueue(nCurrentTask()->requestQueue))
  {
    nTask requestingTask = GetObj(nCurrentTask()->requestQueue);
    if (requestingTask->status == WAIT_SEND || requestingTask->status == WAIT_SEND_TIMEOUT)
    {
      nCurrentTask()->status = nPrintf("%s%sAnswering request for task %s\n", DEBUG,
                                       context, requestingTask->taskname);
      if (requestingTask->status == WAIT_SEND_TIMEOUT)
      {
        CancelTask(requestingTask);
        nPrintf("%s%sCanceling task %s because it reached it's timeout\n",
                DEBUG, context, requestingTask->taskname);
      }
      requestingTask->status = READY;
      requestingTask->send.msg = data;
      PushTask(ready_queue, requestingTask);
      nPrintf("%s%sAdded %s to the ready queue\n", DEBUG, context,
              requestingTask->taskname);
    }
  }
  nPrintf("%s%sPending requests: %d\n", DEBUG, context, pendingRequests);
  while (pendingRequests)
  {
    nCurrentTask()->status = WAIT_REPLY;
    nPrintf("%s%sWaiting for all requests to release the data\n", DEBUG, context);
    ResumeNextReadyTask();
  }
  nPrintf("%s%s%s finished sharing\n", DEBUG, context, nGetTaskName());
  END_CRITICAL();
  nPrintf("%s%sExited critical section.\n\n", DEBUG, context);
}
