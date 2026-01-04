#pragma once
#include "CloseTag.h"

// 主图上画收盘价标签
class CCloseTag_Main : public CCloseTag
{
	Q_OBJECT

public:
	CCloseTag_Main(QCustomPlot* parent);
	virtual ~CCloseTag_Main() override = default;


	// 得到坐标系
	virtual QCPAxisQPtr		GetX() override final;
	virtual QCPAxisQPtr		GetY() override final;

protected:

	virtual void			DrawTag(const KlinePlotSuit& klinePlotSuit) override final;



};

