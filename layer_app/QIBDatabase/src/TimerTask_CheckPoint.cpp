#include "pch.h"
#include "TimerTask_CheckPoint.h"
#include <Global.h>
#include "Factory.h"

CTimerTask_CheckPoint::CTimerTask_CheckPoint()
{
}

CTimerTask_CheckPoint::~CTimerTask_CheckPoint(void)
{  
}

void CTimerTask_CheckPoint::runTimerTask()
{
	MakeAndGet_Env()->Flush();
}
