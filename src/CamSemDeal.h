#ifndef __CAM_SEM_DEAL_H__
#define __CAM_SEM_DEAL_H__

#include <Windows.h>

using namespace std;

#define CAM_SEM_WAIT_FOREVER			0xFFFFFFFF
#define CAM_SEM_WAIT_10MS				10

#define CAM_SEM_OK				0
#define CAM_SEM_ERROR			-1
#define CAM_SEM_TIMEOUT			-2

#define CAM_SEM_INVALID			0
#define CAM_SEM_VALID			1

class CCSem
{
public:
	int sem_flag;
	HANDLE sem;
public:
	int hl_sem_init(unsigned int sem_count, unsigned int max_sem_count);
	int hl_sem_close(void);
	int hl_sem_post(void);
	int hl_sem_wait_forever(void);
	int hl_sem_wait(unsigned int wait_ms);
	int hl_sem_flush_close(void);
	void hl_sem_wait_once(void);
public:
	CCSem();
	~CCSem();
};
#endif
