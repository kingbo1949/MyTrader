#pragma once

#include "CloseTag.h"

// 副图上画收盘价标签
class CCloseTag_Sub : public CCloseTag
{
	Q_OBJECT

public:
	CCloseTag_Sub(QCustomPlot* parent);
	virtual ~CCloseTag_Sub() override = default;


	// 得到坐标系
	virtual QCPAxisQPtr		GetX() override final;
	virtual QCPAxisQPtr		GetY() override final;

protected:

	virtual void			DrawTag(const KlinePlotSuit& klinePlotSuit) override final;

	// 得到需要显示的值
	double					GetShowNumber(const CodeStr& codeId, Time_Type timeType, Tick_T lastTime);

};

