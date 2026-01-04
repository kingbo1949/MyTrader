#pragma once
#include "../src/PlotObject.h"
#include "../src/Factory_AnalystGui.h"
#include <base_struc.h>
#include <AnalystDealDefine.h>
class CItemsHandler : public CPlotObject
{
	Q_OBJECT

public:
	CItemsHandler(QCustomPlot* parent);
	virtual ~CItemsHandler() { ; };

	static void				PrintRange(QCustomPlot* customPlot);

	// 鼠标移动
	virtual void			MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit) = 0;

	// 坐标变化事件
	virtual void			AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit) = 0;

	// K线全部更新时间
	virtual void			SetKLines(const KlinePlotSuit& klinePlotSuit) = 0;

	// 盘中K线更新
	virtual void			UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount) = 0;

	// 全部更新回合
	virtual void			SetMatches(const KlinePlotSuit& klinePlotSuit) = 0;

	// 副图类别改变
	virtual void			SubTypeChg(const KlinePlotSuit& klinePlotSuit) {;};

	// 得到坐标系
	virtual QCPAxisQPtr		GetX() = 0;
	virtual QCPAxisQPtr		GetY() = 0;

protected:
	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) = 0;

	// 比较两个点的高低，把高点移动到新高，低点移动到新低
	void					MovePoints(QPointF& point1, QPointF& point2, double upper, double lower);





};

typedef CItemsHandler* ItemsHandlerQPtr;
typedef std::vector<CItemsHandler*> ItemsHandlerQPtrs;