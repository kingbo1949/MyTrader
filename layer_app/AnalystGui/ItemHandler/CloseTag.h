#pragma once
#include "ItemsHandler.h"
#include "../src/axistag.h"
class CCloseTag : public CItemsHandler
{
	Q_OBJECT

public:
	CCloseTag(QCustomPlot* parent);
	virtual ~CCloseTag() { ; };

	// 鼠标移动
	virtual void			MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit) override final { ; };

	// 坐标变化事件
	virtual void			AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit) override final { ; };

	// K线改变或者回合改变事件
	virtual void			SetKLines(const KlinePlotSuit& klinePlotSuit) override final ;

	// 盘中K线更新
	virtual void			UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount) override final ;

	// 全部更新回合
	virtual void			SetMatches(const KlinePlotSuit& klinePlotSuit) override final { ; };

	// 得到坐标系
	virtual QCPAxisQPtr		GetX() = 0;
	virtual QCPAxisQPtr		GetY() = 0;





protected:
	AxisTagQPtr				m_axisTag = nullptr;

	AxisTagQPtr				MakeAndGet_AxisTag();

	virtual void			DrawTag(const KlinePlotSuit& klinePlotSuit) = 0;

	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override final { return true; };


};

