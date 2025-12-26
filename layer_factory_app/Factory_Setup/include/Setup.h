#pragma once

#include <base_struc.h>
class CSetup
{
public:
	CSetup() { ; };
	virtual ~CSetup() { ; };

	virtual void				Open() = 0;

	// 关闭
	virtual void				Close() = 0;

protected:

};
typedef std::shared_ptr<CSetup> SetupPtr;

