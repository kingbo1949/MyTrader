#pragma once
#include "ItemsHandler.h"
#include "../src/axistag.h"
class CCloseTag : public CItemsHandler
{
	Q_OBJECT

public:
	CCloseTag(QCustomPlot* parent);
	virtual ~CCloseTag() ;

	// 鼠标移动
	virtual void			MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit) final { ; };

	// 坐标变化事件
	virtual void			AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit) final { ; };

	// K线改变或者回合改变事件
	virtual void			SetKLines(const KlinePlotSuit& klinePlotSuit) final ;

	// 盘中K线更新
	virtual void			UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount) final ;

	// 全部更新回合
	virtual void			SetMatches(const KlinePlotSuit& klinePlotSuit) final { ; };

	// 副图类别改变
	virtual void			SubTypeChg(const KlinePlotSuit& klinePlotSuit) final { DrawTag(klinePlotSuit) ;};


	// 得到坐标系
	virtual QCPAxisQPtr		GetX() override = 0 ;
	virtual QCPAxisQPtr		GetY() override = 0;

protected:
	QPointer<AxisTag>		m_axisTag = nullptr;

	QPointer<AxisTag>		MakeAndGet_AxisTag();

	virtual void			DrawTag(const KlinePlotSuit& klinePlotSuit) = 0;

	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override final { return true; };


};

