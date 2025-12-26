#pragma once
#include "TSideTimeHandler.h"
class CTSideTimeHandler_Stock : public CTSideTimeHandler
{
public:
	CTSideTimeHandler_Stock();
	virtual ~CTSideTimeHandler_Stock() { ; };

	// 得到指定区域的颜色
	virtual QColor					GetTSideColor(TradeSideType type) override;


protected:
	// 得到某一时点的区间
	virtual TradeSideType			GetTSideType(Tick_T time) override;

protected:
	Tick_T							m_open;			// 日盘开盘
	Tick_T							m_close;		// 日盘收盘

	Tick_T							m_afterOpen;	// 盘后收盘
	Tick_T							m_afterClose;	// 盘后收盘

	Tick_T							m_nightOpen;	// 夜盘收盘
	Tick_T							m_nightClose;	// 夜盘收盘


};

