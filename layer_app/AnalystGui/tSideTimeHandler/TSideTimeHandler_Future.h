#pragma once
#include "TSideTimeHandler.h"

class CTSideTimeHandler_Future : public CTSideTimeHandler
{
public:
	CTSideTimeHandler_Future();
	virtual ~CTSideTimeHandler_Future() { ; };

	// 得到指定区域的颜色
	virtual QColor					GetTSideColor(TradeSideType type) override;


protected:
	// 得到某一时点的区间
	virtual TradeSideType			GetTSideType(Tick_T time) override;

protected:
	Tick_T							m_open;			// 日盘开盘
	Tick_T							m_close;		// 日盘收盘

	Tick_T							m_afterOpen;	// 盘后开盘
	Tick_T							m_afterClose;	// 盘后收盘

	Tick_T							m_night1Open;	// 夜盘1开盘
	Tick_T							m_night1Close;	// 夜盘1收盘

	Tick_T							m_night2Open;	// 夜盘2开盘
	Tick_T							m_night2Close;	// 夜盘2收盘

	Tick_T							m_night3Open;	// 夜盘3开盘
	Tick_T							m_night3Close;	// 夜盘3收盘
};

