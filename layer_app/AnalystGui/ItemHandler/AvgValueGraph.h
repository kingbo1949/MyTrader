#pragma once
#include "ItemsHandler.h"
class CAvgValueGraph : public CItemsHandler
{
	Q_OBJECT

public:
	CAvgValueGraph(QCustomPlot* parent);
	virtual ~CAvgValueGraph() { ; };

	// 鼠标移动
	virtual void			MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit) override final { ; };

	// 坐标变化事件
	virtual void			AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit) override final { ; };

	// K线改变或者回合改变事件
	virtual void			SetKLines(const KlinePlotSuit& klinePlotSuit) override final;

	// 盘中K线更新
	virtual void			UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount) override final;

	// 全部更新回合
	virtual void			SetMatches(const KlinePlotSuit& klinePlotSuit) override final { ; };

	// 得到坐标系
	virtual QCPAxisQPtr		GetX() override final;
	virtual QCPAxisQPtr		GetY() override final;



protected:
	QCPGraphQPtr			m_graph5;
	QCPGraphQPtr			m_graph20;
	QCPGraphQPtr			m_graph60;
	QCPGraphQPtr			m_graph200;

	std::map<Tick_T, IBAvgValuePtr>	m_mapAvgValue;

	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) = 0;

	void					SetKLines(Time_Type timeType, const IBKLinePtrs& klines, int maCircle);
	void					UpdateKlines(Time_Type timeType, const IBKLinePtrs& klines, int maCircle, int addcount, int chgcount);
	QCPGraphQPtr			GetGraphLine(int circle);


	// 删除最后n个数据
	void					DelCurveData(int circle, int count);

	// 取得对应circle在value中的引用
	double& GetCircleValue(IBAvgValuePtr value, int circle)
	{
		if (circle == 5) return value->v5;
		if (circle == 20) return value->v20;
		if (circle == 60) return value->v60;
		if (circle == 200) return value->v200;

		printf("error circle, circle = %d \n", circle);
		exit(-1);
		return value->v5;
	}



};

