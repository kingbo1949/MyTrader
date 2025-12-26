#pragma once

class CTimerTask_CheckPoint : public TimerTask
{
public:
	CTimerTask_CheckPoint();
	virtual ~CTimerTask_CheckPoint(void);

	virtual void				runTimerTask();

};
