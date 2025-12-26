#pragma once

// 回报对象的抽象基类
#include <base_trade.h>
class COnRtn
{
public:
	COnRtn() { ; };
	virtual ~COnRtn() { ; };

	virtual void				Execute() = 0;

};

typedef std::shared_ptr<COnRtn> OnRtnPtr;
