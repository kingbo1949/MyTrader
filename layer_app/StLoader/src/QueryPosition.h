#pragma once
#include <base_trade.h>
class CQueryPosition
{
public:
	CQueryPosition() { ; };
	virtual ~CQueryPosition() { ; };

	void			GoQuery();

protected:

	// 直接按持仓查询
	void			QueryAllPs();

	// 按策略查询持仓
	void			QueryPs(StrategyParamPtr strategy);

	// 持仓带正负号的字符串表示
	std::string		GetPsVolStr(double vol, LongOrShort longOrShort);
};

