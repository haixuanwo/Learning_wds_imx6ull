/*
 * @Author: Clark
 * @Email: haixuanwoTxh@gmail.com
 * @Date: 2025-12-30 16:24:09
 * @LastEditors: Clark
 * @LastEditTime: 2025-12-30 17:30:24
 * @Description: file content
 */
#include "input_manager.h"

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

#include <input_manager.h>

static PInputDevice g_InputDevs  = NULL;

static pthread_mutex_t g_tMutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_tConVar = PTHREAD_COND_INITIALIZER;


/* start of 实现环形buffer */
#define BUFFER_LEN 20
static int g_iRead  = 0;
static int g_iWrite = 0;
static InputEvent g_atInputEvents[BUFFER_LEN];


static int isInputBufferFull(void)
{
	return (g_iRead == ((g_iWrite + 1) % BUFFER_LEN));
}

static int isInputBufferEmpty(void)
{
	return (g_iRead == g_iWrite);
}

static void PutInputEventToBuffer(PInputEvent ptInputEvent)
{
	if (!isInputBufferFull())
	{
		g_atInputEvents[g_iWrite] = *ptInputEvent;
		g_iWrite = (g_iWrite + 1) % BUFFER_LEN;
	}
}


static int GetInputEventFromBuffer(PInputEvent ptInputEvent)
{
	if (!isInputBufferEmpty())
	{
		*ptInputEvent = g_atInputEvents[g_iRead];
		g_iRead = (g_iRead + 1) % BUFFER_LEN;
		return 1;
	}
	else
	{
		return 0;
	}
}


/* end of 实现环形buffer */


void RegisterInputDevice(PInputDevice ptInputDev)
{
    printf("T --- %s %d name[%s]\n", __func__, __LINE__, ptInputDev->name);

	ptInputDev->ptNext = g_InputDevs;
	g_InputDevs = ptInputDev;
}

static void *input_recv_thread_func (void *data)
{
	PInputDevice ptInputDev = (PInputDevice)data;
	InputEvent tEvent;
	int ret;

	while (1)
	{
		/* 读数据 */
		ret = ptInputDev->GetInputEvent(&tEvent);

		if (!ret)
		{
			/* 保存数据 */
			pthread_mutex_lock(&g_tMutex);
			PutInputEventToBuffer(&tEvent);

			/* 唤醒等待数据的线程 */
			pthread_cond_signal(&g_tConVar); /* 通知接收线程 */
            printf("T --- signal %s %d name[%s]\n", __func__, __LINE__, ptInputDev->name);

			pthread_mutex_unlock(&g_tMutex);
		}
	}

	return NULL;
}

void IntpuDeviceInit(void)
{
	int ret;
	pthread_t tid;

	/* for each inputdevice, init, pthread_create */
	PInputDevice ptTmp = g_InputDevs;
	while (ptTmp)
	{
		/* init device */
		ret = ptTmp->DeviceInit();

		/* pthread create */
		if (!ret)
		{
            printf("T --- %s %d pthread_create name[%s]\n", __func__, __LINE__, ptTmp->name);
			ret = pthread_create(&tid, NULL, input_recv_thread_func, ptTmp);
		}

		ptTmp= ptTmp->ptNext;
	}
}

int GetInputEvent(PInputEvent ptInputEvent)
{
	InputEvent tEvent;
	int ret;
	/* 无数据则休眠 */
	pthread_mutex_lock(&g_tMutex);
	if (GetInputEventFromBuffer(&tEvent))
	{
		*ptInputEvent = tEvent;
		pthread_mutex_unlock(&g_tMutex);
        printf("T --- %s %d type[%d]\n", __func__, __LINE__, ptInputEvent->type);

		return 0;
	}
	else
	{
		/* 休眠等待 */
		pthread_cond_wait(&g_tConVar, &g_tMutex);
		if (GetInputEventFromBuffer(&tEvent))
		{
			*ptInputEvent = tEvent;
            printf("T --- %s %d type[%d]\n", __func__, __LINE__, ptInputEvent->type);
			ret = 0;
		}
		else
		{
			ret = -1;
		}
		pthread_mutex_unlock(&g_tMutex);
	}

	return ret;
}
