#pragma once

#include <base_trade.h>
class CNeedCancel
{
public:
	CNeedCancel() { ; };
	virtual ~CNeedCancel() { ; };

	virtual bool					CanCancel() = 0;

};

typedef std::shared_ptr<CNeedCancel> NeedCancelPtr;
