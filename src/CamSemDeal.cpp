#include "CamSemDeal.h"

#define CAM_DEFAULT_SEM_WAIT_TIME				10

CCSem::CCSem()
{
	sem_flag = CAM_SEM_INVALID;
	sem = NULL;
}

CCSem::~CCSem()
{

}

int CCSem::hl_sem_init(unsigned int sem_count, unsigned int max_sem_count)
{
	if (sem_flag == CAM_SEM_VALID)
	{
		return CAM_SEM_OK;
	}

	sem = CreateSemaphore(NULL, sem_count, max_sem_count, NULL);

	if (sem == NULL)
	{
		return CAM_SEM_ERROR;
	}

	sem_flag = CAM_SEM_VALID;
	return CAM_SEM_OK;
}

int CCSem::hl_sem_close(void)
{
	if (sem_flag == CAM_SEM_INVALID)
	{
		return CAM_SEM_OK;
	}

	CloseHandle(sem);

	sem_flag = CAM_SEM_INVALID;
	return CAM_SEM_OK;
}

int CCSem::hl_sem_post(void)
{
	if (sem_flag == CAM_SEM_INVALID)
	{
		return CAM_SEM_ERROR;
	}

	ReleaseSemaphore(sem, 1, NULL);

	return CAM_SEM_OK;
}

int CCSem::hl_sem_wait_forever(void)
{
	if (sem_flag == CAM_SEM_INVALID)
	{
		return CAM_SEM_ERROR;
	}

	while (1)
	{
		DWORD ret = WaitForSingleObject(sem, CAM_DEFAULT_SEM_WAIT_TIME);
		if (ret == WAIT_FAILED)
		{
			return CAM_SEM_ERROR;
		}
		if (ret == WAIT_OBJECT_0)
		{
			return CAM_SEM_OK;
		}
		if (sem_flag == CAM_SEM_INVALID)
		{
			return CAM_SEM_ERROR;
		}
	}
	return CAM_SEM_ERROR;
}

int CCSem::hl_sem_wait(unsigned int wait_ms)
{
	if (sem_flag == CAM_SEM_INVALID)
	{
		return CAM_SEM_ERROR;
	}

	if (wait_ms == 0)
	{
		return CAM_SEM_ERROR;
	}

	if (wait_ms == CAM_SEM_WAIT_FOREVER)
	{
		return hl_sem_wait_forever();
	}

	int wait_cnt = wait_ms / CAM_DEFAULT_SEM_WAIT_TIME;
	int wait_last_time = wait_ms % CAM_DEFAULT_SEM_WAIT_TIME;

	for (int i = 0; i < wait_cnt; i++)
	{
		DWORD ret = WaitForSingleObject(sem, CAM_DEFAULT_SEM_WAIT_TIME);
		if (ret == WAIT_FAILED)
		{
			return CAM_SEM_ERROR;
		}
		if (ret == WAIT_OBJECT_0)
		{
			return CAM_SEM_OK;
		}
		if (sem_flag == CAM_SEM_INVALID)
		{
			return CAM_SEM_ERROR;
		}
	}

	if (wait_last_time > 0)
	{
		DWORD ret = WaitForSingleObject(sem, wait_last_time);
		if (ret == WAIT_FAILED)
		{
			return CAM_SEM_ERROR;
		}
		if (ret == WAIT_OBJECT_0)
		{
			return CAM_SEM_OK;
		}
		if (sem_flag == CAM_SEM_INVALID)
		{
			return CAM_SEM_ERROR;
		}
	}

	return CAM_SEM_TIMEOUT;
}

int CCSem::hl_sem_flush_close(void)
{
	if (sem_flag == CAM_SEM_INVALID)
	{
		return CAM_SEM_ERROR;
	}

	sem_flag = CAM_SEM_INVALID;
	ReleaseSemaphore(sem, 1, NULL);
	CloseHandle(sem);

	return CAM_SEM_OK;
}

void CCSem::hl_sem_wait_once(void)
{
	hl_sem_wait(CAM_DEFAULT_SEM_WAIT_TIME);
}

