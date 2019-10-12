#include <nSystem.h>

/* Ver el nMain al final */

int verbose = FALSE;

typedef struct
{
  int start, end;
  int shareCnt, relCnt;
  nMonitor mutex;
  int finish, finish0;
} Data;

void iniData(Data *pdata)
{
  pdata->start = pdata->end = -1;
  pdata->shareCnt = pdata->relCnt = 0;
  pdata->mutex = nMakeMonitor();
}

void freeData(Data *pdata)
{
  if (pdata->shareCnt != pdata->relCnt)
    nFatalError("freeData", "numero de nRequest y nRelease no coincide\n");
  nDestroyMonitor(pdata->mutex);
}

/****************************************************
 * Procedimientos de prueba y verificacion
 ****************************************************/

/*  reqProc:
 *  Invoca nRequest(shareTask, timeout) en T= idle milisegs.
 *  Cuando nRequest retorna, espera busy milisegs. e invoca nRelease(shareTask).
 */
static int reqProc(nTask shareTask, Data *pdata,
                   int idle, int busy, int timeout)
{
  int start;
  int id;
  char *share_data;
  nSleep(idle);
  start = nGetTime();

  if (verbose)
    nPrintf("begin request\n");
  share_data = (char *)nRequest(shareTask, timeout);
  if (share_data == NULL)
    return -1; /* Timeout vencido */
  if (share_data != (char *)pdata)
    nFatalError("reqProc", "El dato compartido no coincide\n");

  if ((start >= pdata->start && nGetTime() - start > 5) || (start < pdata->start && nGetTime() - pdata->start > 5))
    nFatalError("reqProc", "nRequest tardo mucho en responder\n");
  nEnter(pdata->mutex);
  id = ++pdata->shareCnt;
  if (verbose)
    nPrintf("end request %d\n", id);
  nExit(pdata->mutex);
  nSleep(busy);
  nEnter(pdata->mutex);
  pdata->relCnt++;
  if (verbose)
    nPrintf("release id=%d released=%d\n", id, pdata->relCnt);
  nExit(pdata->mutex);
  pdata->end = nGetTime();

  nRelease(shareTask);

  return 0;
}

/*
 * Test trivial: se llama a nShare y no hay nRequest's esperando.
 * Hay que retornar de inmediato.
 */
int test0()
{
  int data;
  nShare((char *)&data);
  nPrintf("Ok\n");
  return 0;
}

/*
 * Test simple: se llama a nShare y hay un nRequest esperando.
 * Si hay timeout, este no se vence.
 *   t1           t2
 *   |            |
 *   |             \ nRequest(t1, timeout)   T= 0
 *   |              :                    
 *    \ nShare      :                        T= delay
 *    :            /
 *    :           |
 *    :           |
 *    :____________> nRelease(t1)            T= delay+10   
 *    /           |
 *   |            |
 */
int test1(int delay, int timeout)
{
  nTask t;
  Data data;
  int start;
  iniData(&data);
  t = nEmitTask(reqProc, nCurrentTask(), &data, 0, 10, timeout);
  nSleep(delay);
  data.start = nGetTime();
  if (verbose)
    nPrintf("begin sharing\n");
  nShare((char *)&data);
  if (verbose)
    nPrintf("end sharing %d\n", data.shareCnt);
  if (data.relCnt != 1)
    nFatalError("test1", "nShare retorno sin un nRelease\n");
  if (nGetTime() - data.end > 5) /* En realidad deberia ser 0 */
    nFatalError("test1", "nRelease tarda demasiado en activar nShare\n");
  start = nGetTime();
  if (nWaitTask(t) != 0)
    nFatalError("test1", "codigo de retorno errado\n");
  if (nGetTime() - start > 5) /* En realidad deberia ser 0 */
    nFatalError("test1", "nRelease tarda demasiado en retornar\n");
  freeData(&data);
  return 0;
}

/*
 * Test doble: se llama a nShare y hay dos nRequest.
 * Si hay timeout, este no se vence.
 *   t1           t2                       t3
 *   |            |                        |
 *   |             \ nRequest(t1, timeout) |       T= 0 (llamada de nRequest)
 *   |              :                      |       T= x (retorno de nRequest)
 *    \ nShare      :                      |       T= delay1 (llamada a nShare)
 *    :            /                        \nRequest(t1, timeout) T=delay2
 *    :           |                         /      T= y (retorno de nRequest)
 *    :           |                        |
 *    :            > nRelease(t1) T=x+busy1|
 *    :___________|_________________________> nRelease(t1, timeout) T=y+busy2
 *    /           |                        |
 *   |            |                        |
 */
int test2(int delay1, int delay2, int busy1, int busy2, int timeout)
{
  nTask t1, t2;
  Data data;
  int start;
  iniData(&data);
  t1 = nEmitTask(reqProc, nCurrentTask(), &data, 0, busy1, timeout);
  t2 = nEmitTask(reqProc, nCurrentTask(), &data, delay2, busy2, timeout);
  nSleep(delay1);
  data.start = nGetTime();
  if (verbose)
    nPrintf("begin sharing\n");
  nShare((char *)&data);
  if (verbose)
    nPrintf("end sharing %d\n", data.shareCnt);
  if (data.relCnt != 2)
    nFatalError("test2", "nShare retorno sin un nRelease\n");
  if (nGetTime() - data.end > 5)
    nFatalError("test2", "nRelease tarda demasiado en activar nShare\n");
  start = nGetTime();
  if (nWaitTask(t1) != 0)
    nFatalError("test2", "codigo de retorno de t1 errado\n");
  if (nWaitTask(t2) != 0)
    nFatalError("test2", "codigo de retorno de t2 errado\n");
  if (nGetTime() - start > 5)
    nFatalError("test2", "nRelease tarda demasiado en retornar\n");
  freeData(&data);
  return 0;
}

/* test3: verifica que el timeout se venza, despues de timeout milisegs.
 *  t2
 *  |
 *   \ nRequest(t1, timeout)  T= 0
 *   :  (no hay nShare)
 *   :
 *   /  T >= timeout
 *   |
 */
int test3(int timeout)
{
  nTask t;
  int end;
  int start = nGetTime();
  t = nEmitTask(reqProc, nCurrentTask(), NULL, 0, 1, timeout);
  if (nWaitTask(t) != -1)
    nFatalError("test2", "codigo de retorno errado\n");
  end = nGetTime();
  if (end - start < timeout)
    nFatalError("test", "el timeout no se cumple\n");
  if (end - start > timeout + 5)
    nFatalError("test", "el timeout es mayor al esperado\n");
  return 0;
}

/* reqProcLoop se usa en test4 */

int reqProcLoop(nTask t, int n, nMonitor shareMutex, nMonitor relMutex)
{
  Data *pdata;
  do
  {
    pdata = (Data *)nRequest(t, 1000000);
    if (pdata == NULL)
      nFatalError("reqProcLoop", "pdata es NULL\n");
    nEnter(shareMutex);
    pdata->shareCnt++;
    if (pdata->shareCnt == n)
    {
      pdata->shareCnt = 0;
      nNotifyAll(shareMutex);
    }
    else
      nWait(shareMutex);
    nExit(shareMutex);

    nRelease(t);

    nEnter(relMutex);
    pdata->relCnt++;
    if (pdata->relCnt == n)
    {
      pdata->relCnt = 0;
      if (pdata->finish0)
        pdata->finish = TRUE;
      nNotifyAll(relMutex);
    }
    else
      nWait(relMutex);
    nExit(relMutex);
  } while (!pdata->finish);
  return 0;
}

/* timeDisplay solo para mostrar como avanza el tiempo */

int terminados = 0;

int timeDisplay(int delay)
{
  int curr = nGetTime();
  int show = curr + 1000;
  int segs = 0;
  do
  {
    nSleep(5);
    curr = nGetTime();
    if (curr > show)
    {
      segs++;
      nPrintf("%d: terminados= %d\n", segs, terminados);
      show = show + 1000;
    }
  } while (terminados < 100);
  nPrintf("%d: terminados= %d\n", segs, terminados);
  return 0;
}

/* test4 es para el modo preemptive.
 * - Se lanzan n tareas que ejecutan reqProcLoop
 * - Se invoca nShare
 * - Se espera a que todas las tareas invoquen nRequest
 * - Se espera a que todas las tareas invoquen nRelease
 * - Se repite esto hasta que transcurran delay milisegs.
 */

int test4(int n, int delay)
{
  nTask *tasks = (nTask *)nMalloc(sizeof(nTask) * n);
  int i, iter;
  nTask self = nCurrentTask();
  int start = nGetTime();
  int end = start + delay;
  nMonitor shareMutex = nMakeMonitor();
  nMonitor relMutex = nMakeMonitor();
  Data data;
  data.finish = FALSE;
  data.finish0 = FALSE;
  iniData(&data);
  iter = 0;
  data.shareCnt = data.relCnt = 0;
  for (i = 0; i < n; i++)
    tasks[i] = nEmitTask(reqProcLoop, self, n, shareMutex, relMutex);
  do
  {
    data.start = nGetTime();

    nShare((char *)&data);

    iter++;
    if (verbose)
      nPrintf("Iter= %d\n", iter);
  } while (data.start < end);
  data.finish0 = TRUE;
  data.start = nGetTime();
  while (!data.finish)
  {
    nSleep(100);
    nShare((char *)&data);
  }
  for (i = 0; i < n; i++)
    nWaitTask(tasks[i]);
  freeData(&data);
  nDestroyMonitor(shareMutex);
  nDestroyMonitor(relMutex);
  return 0;
}

/****************************************************
 * Programa principal
 ****************************************************/

int nMain(int argc, char **argv)
{
  nPrintf("nShare sin nRequest pendientes retorna de inmediato\n");
  test0();
  nPrintf("Ok\n");

  nPrintf("nShare con un nRequest que espera\n");
  test1(10, -1);
  nPrintf("Ok\n");

  nPrintf("nShare con 2 nRequest uno espera, el otro no. nRelease en orden.\n");
  test2(10, 15, 10, 10, -1);
  nPrintf("Ok\n");

  nPrintf("nShare con 2 nRequest uno espera, el otro no. %s\n",
          "nRelease en orden inverso");
  test2(10, 15, 20, 5, -1);
  nPrintf("Ok\n");

  nPrintf("nShare con 2 nRequest esperando.\n");
  test2(10, 5, 10, 10, -1);
  nPrintf("Ok\n");

  nPrintf("nShare con un nRequest con timeout que no se vence\n");
  test1(50, 100);
  nPrintf("Ok\n");

  nPrintf("nShare con 2 nRequest con timeout que no se vencen\n");
  test2(10, 15, 10, 10, 200);
  nPrintf("Ok\n");

  nPrintf("nShare con un nRequest con timeout que si se vence\n");
  test3(50);
  nPrintf("Ok\n");

  nPrintf("Los 7 tests anteriores en paralelo\n");
  {
    nTask t0, t1, t2, t3, t4, t5, t6, t7;
    t0 = nEmitTask(test1, 10, -1);
    nSleep(5);
    t1 = nEmitTask(test2, 10, 15, 10, 10, -1);
    nSleep(5);
    t2 = nEmitTask(test2, 10, 15, 20, 5, -1);
    nSleep(5);
    t3 = nEmitTask(test2, 10, 5, 10, 10, -1);
    nSleep(5);
    t4 = nEmitTask(test3, 50);
    nSleep(5);
    t5 = nEmitTask(test1, 50, 100);
    nSleep(5);
    t6 = nEmitTask(test2, 10, 15, 10, 10, 200);
    nSleep(5);
    t7 = nEmitTask(test0);
    nWaitTask(t0);
    nWaitTask(t1);
    nWaitTask(t2);
    nWaitTask(t3);
    nWaitTask(t4);
    nWaitTask(t5);
    nWaitTask(t6);
    nWaitTask(t7);
  }
  nPrintf("Ok\n");

  nSetTimeSlice(1);
  nPrintf("Test de robustez: dura mas o menos 1 minuto\n");
  {
    int delay = 10000;
    int ntasks = 100;
    int nreqs = 100;
    nTask timeTask = nEmitTask(timeDisplay, delay);
    nTask *tasks = (nTask *)nMalloc(sizeof(nTask) * ntasks);
    int i;
    for (i = 0; i < ntasks; i++)
      tasks[i] = nEmitTask(test4, nreqs, delay);
    for (i = 0; i < ntasks; i++)
    {
      nWaitTask(tasks[i]);
      terminados++;
    }
    nWaitTask(timeTask);
  }
  nPrintf("Ok\n");
  nPrintf("Felicitaciones: su tarea paso todos los tests.\n");
  return 0;
}
