#pragma once
#include "ItemsHandler.h"
#include "../src/Factory_AnalystGui.h"
// 用不同的背景色标注盘后与夜盘时间

class CTradeSideRect : public CItemsHandler
{
	Q_OBJECT

public:
	CTradeSideRect(QCustomPlot* parent);
	virtual ~CTradeSideRect() { ; };

	// 鼠标移动
	virtual void			MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit) override { ; };

	// 坐标变化事件
	virtual void			AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit)  override;

	// K线改变或者回合改变事件
	virtual void			SetKLines(const KlinePlotSuit& klinePlotSuit)  override;

	// 盘中K线更新
	virtual void			UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount) override { ; };

	// 全部更新回合
	virtual void			SetMatches(const KlinePlotSuit& klinePlotSuit) override { ; };

	// 得到坐标系
	virtual QCPAxisQPtr		GetX() override;
	virtual QCPAxisQPtr		GetY() override;

protected:
	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override final { return true; };

	// 制作矩形
	QCPItemRectQPtr			MakeOneRect();

	// 从customplot中删除
	void					ClearItems();

	// 扫描k线得到盘外区间
	TSideRanges				ScanKlines(const CodeStr& codeId, const IBKLinePtrs& klines);

	// 绘出区域
	void					DrawRange(SecurityType securityType, const TSideRange& range);

	// 修改左右横坐标
	void					ModifyXCoords(QCPItemRectQPtr rect, double left, double right);
	// 修改上下总坐标
	void					ModifyYCoords(QCPItemRectQPtr rect);

	void					TestDrawRect();



private:

	QCPItemRectQPtrs		m_rects;	// 矩形集合






};

