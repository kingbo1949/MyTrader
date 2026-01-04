#pragma once
#include "SubGraph.h"
#include <Factory_IBGlobalShare.h>

struct DivergenceArrow
{
	DivergenceType	type;// = DivergenceType::Normal;
	bool			isUTurn = false;
	QCPItemLineQPtr	arrow = nullptr;
};

// macd 背离
class CMacdDivergenceItem : public CSubGraph
{
	Q_OBJECT

public:
	CMacdDivergenceItem(QCustomPlot* parent, SubType subType);
	virtual ~CMacdDivergenceItem() { ; };

	// 鼠标移动
	virtual void			MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit) override { ; };

	// 坐标变化事件
	virtual void			AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit) override;

	// K线改变或者回合改变事件
	virtual void			SetKLines(const KlinePlotSuit& klinePlotSuit) override;

	// 盘中K线更新
	virtual void			UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount) override;

	// 全部更新回合
	virtual void			SetMatches(const KlinePlotSuit& klinePlotSuit) override { ; };


protected:
	// 索引到标注背离的箭头的map
	std::map<int, DivergenceArrow>	m_arrows;

	std::map<Tick_T, IBDivTypePtr>	m_mapDivTypeValue;

	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override final;


	// 从customplot中删除
	void					ClearItems();

	void					DelDataAtIndex(int index);

	// 制作箭头对象并加入map
	DivergenceArrow			MakeOneArrow(int index, IBDivTypePtr type);


	// 在指定索引安装背离箭头
	void					SetupArrowAtIndex(int index, DivergenceArrow divergenceArrow);

	// 盘中K线更新
	void					UpdateKlines(Time_Type timeType, const IBKLinePtrs& klines, int addcount, int chgcount);

};

