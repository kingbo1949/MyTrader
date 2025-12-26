#pragma once

// 管理图像上的items

#include <base_struc.h>
#include "ItemsHandler.h"

// 需要随鼠标移动而重画的items
struct CrossLineItems
{
	QCPItemLine* vLine;		// 十字虚线 纵线
	QCPItemLine* hLine;		// 十字虚线 水平线
	QCPItemText* hTextItem;	// 十字虚线注释 显示横轴的值
	QCPItemText* vTextItem;	// 十字虚线注释 显示横轴的值

};
typedef std::shared_ptr<CrossLineItems> CrossLineItemsPtr;

class CMouseMoveItems : public CItemsHandler
{
	Q_OBJECT

public:
	CMouseMoveItems(QCustomPlot* parent);
	~CMouseMoveItems() { ; };

	// 鼠标移动
	virtual void			MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit) override;

	// 坐标变化事件
	virtual void			AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit)  override { ; };

	// K线改变或者回合改变事件
	virtual void			SetKLines(const KlinePlotSuit& klinePlotSuit)  override final { ; };

	// 盘中K线更新
	virtual void			UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount) override { ; };

	// 全部更新回合
	virtual void			SetMatches(const KlinePlotSuit& klinePlotSuit) override { ; };

	// 得到坐标系
	virtual QCPAxisQPtr		GetX() override;
	virtual QCPAxisQPtr		GetY() override;

	QCPAxisQPtr				GetSubX();
	QCPAxisQPtr				GetSubY();

protected:
	void					Init();

	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override final { return true; };


	// 画K线的tooltip
	void					DrawToolTip(const CodeStr& codeId, Time_Type timeType, QMouseEvent* event, const IBKLinePtrs& klines);

	// 画十字线
	void					DrawCrossLine(QMouseEvent* event, const IBKLinePtrs& klines);

private:
	CrossLineItemsPtr		m_crossLineItems;

};

typedef CMouseMoveItems* MouseMoveItemsQPtr;