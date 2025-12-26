#pragma once
#include <memory>
// tick驱动 做一些守护处理
class CDaemonByTick
{
public:
	CDaemonByTick() { ; };
	virtual ~CDaemonByTick() { ; };

	virtual void		Execute() = 0;

};
typedef std::shared_ptr<CDaemonByTick> DaemonByTickPtr; 

