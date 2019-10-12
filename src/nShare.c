/**
 * Implementation of a concurrent sharing system using low level synchronization tools.
 * 
 * @author Ignacio Slater Muñoz
 * @version 1.0b12
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

static const char
    *DEBUG = "DEBUG   ",
    *ERROR = "ERROR   ";

static size_t pendingRequests = 0;
static int
    nShareCounter = 0,
    nReleaseCounter = 0,
    nRequestCounter = 0;

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
  START_CRITICAL();
  const char *context = "[nRequest]   ";
  nSetTaskName("REQUEST %d", nRequestCounter++);
  nPrintf("\n%s%sPending requests: %d\n", DEBUG, context, pendingRequests);
  nPrintf("%s%sEntered critical section.\n", DEBUG, context);
  pendingRequests++;
  nPrintf("%s%sPending requests: %d\n", DEBUG, context, pendingRequests);

  if (t->status == WAIT_RELEASE)
  {
    nCurrentTask()->send.msg = t->send.msg;
    nPrintf("%s%s%s was active, %s returned %X\n", DEBUG, context, t->taskname,
            nGetTaskName(), nCurrentTask()->send.msg);
    // PushTask(ready_queue, nCurrentTask());
    // nPrintf("%s%sAdded %s to the ready queue\n", DEBUG, context, nGetTaskName());
  }
  else
  {
    PushTask(t->send_queue, nCurrentTask());
    nPrintf("%s%sAdded %s to %s's send queue\n", DEBUG, context, nGetTaskName(),
            t->taskname);
    nPrintf("%s%s%s's timeout: %d\n", DEBUG, context, nGetTaskName(), timeout);
    if (timeout > 0)
    {
      nCurrentTask()->status = WAIT_SHARE_TIMEOUT;
      ProgramTask(timeout);
      nPrintf("%s%s%s started a request with timeout: %d\n", DEBUG, context, nGetTaskName(),
              timeout);
    }
    else
    {
      nCurrentTask()->status = WAIT_SHARE;
      nPrintf("%s%s%s started a request without timeout\n", DEBUG, context, nGetTaskName());
    }
    // FIXME: Running task shouldn't be ready
    // Error Fatal en la rutina ResumeNextReadyTask y la tarea
    // REQUEST 6 (estado READY)
    // Por que' la tarea que corria estaba ``ready''?
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

  if (t->status != WAIT_RELEASE)
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
  nPrintf("%s%sExited critical section.\n", DEBUG, context);
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
  while (!EmptyQueue(nCurrentTask()->send_queue))
  {
    nTask requestingTask = GetTask(nCurrentTask()->send_queue);
    if (requestingTask->status == WAIT_SHARE || requestingTask->status == WAIT_SHARE_TIMEOUT)
    {
      nCurrentTask()->status = nPrintf("%s%sAnswering request for task %s\n", DEBUG,
                                       context, requestingTask->taskname);
      if (requestingTask->status == WAIT_SHARE_TIMEOUT)
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
    nCurrentTask()->status = WAIT_RELEASE;
    nPrintf("%s%sWaiting for all requests to release the data\n", DEBUG, context);
    ResumeNextReadyTask();
  }
  nPrintf("%s%s%s finished sharing\n", DEBUG, context, nGetTaskName());
  END_CRITICAL();
  nPrintf("%s%sExited critical section.\n\n", DEBUG, context);
}
