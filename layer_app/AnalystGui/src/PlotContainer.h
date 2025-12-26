#pragma once

// 将klines显示在customPlot中
#include <qobject.h>
#include <base_struc.h>
#include <AnalystDealDefine.h>
#include "PlotObject.h"
#include "qcustomplot.h"
#include "Factory_AnalystGui.h"
#include "../ItemHandler/ItemsHandler.h"
#include "KLineUpdator.h"
class CPlotContainer : public CPlotObject
{
	Q_OBJECT

public:
	CPlotContainer(QCustomPlot* parent);
	~CPlotContainer() { ; };

	// 切换品种或K线周期，被按键所触发调用
	void									SetKlines(const CodeStr& codeId, Time_Type timeType, const IBKLinePtrs& klines);

	// 实时行情中添加数据klines最多两个数据 返回值为新增的数据数目, 被timer所触发调用
	void									AddKlines(const CodeStr& codeId, Time_Type timeType, const IBKLinePtrs& klines);

	void									SetTMatches(SimpleMatchPtrs matches);

	void									onMouseMove(QMouseEvent* event);

	void									GoHome();
	void									GoEnd();



protected:
	KlinePlotSuit							m_klinePlotSuit;
	KLineUpdatorPtr							m_klineUpdator;
	ItemsHandlerQPtrs						m_items;
	QCPMarginGroupQPtr						m_marginGroup;
	XTickerQSPtr							m_textTicker;

	void									InitItems();

	void									onMouseRelease(QMouseEvent* event);
	void									onMouseWheel(QWheelEvent* event);

	// 因为鼠标的拖拽或缩放事件，改变轴
	void									ChangeAxisRangeForMouse();

	// 触发SetKLines事件
	void									PlusSetKLines();

	// 触发盘中数据更新事件
	void									PlusUpdateKlines(int addcount, int chgcount);

	// 触发更新回合事件
	void									PlusSetMatches();



};
typedef CPlotContainer* PlotContainerQPtr;
