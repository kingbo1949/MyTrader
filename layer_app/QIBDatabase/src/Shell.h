#pragma once
#include <Ice/Application.h>
using namespace Ice;
class CShell : public Application
{
public:
	CShell(void);
	virtual ~CShell(void);

	virtual int					run(int, char* []);
	virtual void				interruptCallback(int signal);


protected:
	void						SetupEnv();

protected:
	TimerPtr					m_pTimerJob_UpdateIndex;


};

