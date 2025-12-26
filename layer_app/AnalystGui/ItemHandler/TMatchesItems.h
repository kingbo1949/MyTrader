#pragma once

// 处理回合的类
#include <base_struc.h>
#include <AnalystDealDefine.h>
#include "../src/Factory_AnalystGui.h"
#include "../src/BracketWithArrow.h"
#include "ItemsHandler.h"

// 用于画回合线的结构
struct TMatchPointPair
{
	BuyOrSell		open_buyOrSell;
	QPointF			beginPoint = QPointF(-1, -1);
	QPointF			endPoint = QPointF(-1, -1);
	bool            haveProfit = false;		// 是否盈利
};
typedef std::shared_ptr<TMatchPointPair> TMatchPointPairPtr;

// 使用K线的左边缘还是右边缘
enum class LeftOrRight
{
	Left = 0,
	Right = 1,
	NoMove = 2
};


class CTMatchesItems : public CItemsHandler
{
	Q_OBJECT

public:
	CTMatchesItems(QCustomPlot* parent);
	virtual ~CTMatchesItems() { ; };

	// 鼠标移动
	virtual void			MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit) override { ; };

	// 坐标变化事件
	virtual void			AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit)  override;

	// K线改变或者回合改变事件
	virtual void			SetKLines(const KlinePlotSuit& klinePlotSuit)  override { ; };

	// 盘中K线更新
	virtual void			UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount) override { ; };

	// 全部更新回合
	virtual void			SetMatches(const KlinePlotSuit& klinePlotSuit) override;

	// 得到坐标系
	virtual QCPAxisQPtr		GetX() override;
	virtual QCPAxisQPtr		GetY() override;





protected:
	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override final { return true; };

	// 按交易回合得到起点终点坐标，准备画线
	TMatchPointPairPtr		MakeTMatchPair(Time_Type timeType, const IBKLinePtrs klines, SimpleMatchPtr oneMatch);

	void					DrawOneMatch(TMatchPointPairPtr tMatchPointPair);
	void					DrawOneMatch_Same(TMatchPointPairPtr tMatchPointPair);	// 开平仓处于同一根K线
	void					DrawOneMatch_Diff(TMatchPointPairPtr tMatchPointPair);	// 开平仓处于不同K线

	// 在图像顶部画买卖箭头
	void					DrawTopArrow(BuyOrSell buyOrSell, double x, LeftOrRight leftOrRight);

	// 根据k线宽度调整端点
	QPointF					ModifyPoint(QPointF point, LeftOrRight leftOrRight);

	// 制作顶部箭头
	QCPItemLineQPtr			MakeOneTopArrow();

	// 制作中括号回合箭头
	BracketWithArrowQPtr	MakeOneBracketWithArrow();

	// 制作普通回合箭头
	QCPItemLineQPtr			MakeOneMatchArrow();

	// 从customplot中删除
	void					ClearItems();


private:
	QCPItemLineQPtrs		m_topArrows;	// 顶部的回合箭头
	QCPAbstractItemQPtrs	m_matchArrows;	// 回合箭头



};

typedef CTMatchesItems* TMatchesItemsQPtr;