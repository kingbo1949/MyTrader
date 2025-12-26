#pragma once
#include "ItemsHandler.h"
#include <Calculator_Macd.h>
#include "../src/Factory_AnalystGui.h"


class CMacdGraph : public CItemsHandler
{
	Q_OBJECT

public:
	CMacdGraph(QCustomPlot* parent, XTickerQSPtr textTicker);
	virtual ~CMacdGraph() { ; };

	// 鼠标移动
	virtual void			MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit) override final { ; };

	// 坐标变化事件
	virtual void			AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit) override final;

	// K线改变或者回合改变事件
	virtual void			SetKLines(const KlinePlotSuit& klinePlotSuit) override final;

	// 盘中K线更新
	virtual void			UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount) override final;

	// 全部更新回合
	virtual void			SetMatches(const KlinePlotSuit& klinePlotSuit) override final { ; };

	// 得到坐标系
	virtual QCPAxisQPtr		GetX() override;
	virtual QCPAxisQPtr		GetY() override;

protected:
	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override final;




private:
	std::map<Tick_T, IBMacdPtr>	m_mapMacdValue;
	XTickerQSPtr 			m_textTicker;
	QCPGraphQPtr			m_macd;
	QCPGraphQPtr			m_dea;
	QCPBarsQPtr				m_macdBarPositive; // 正值
	QCPBarsQPtr				m_macdBarNegative; // 负值
	void					InitAxis();

	// 从横轴的范围得到数据范围的索引
	void					GetWindowsPos(int& beginPos, int& endPos);

	// [xBeginPos, xEndPos)内m_difValues的高低点
	virtual void			GetHighLow(Tick_T xBeginPos, Tick_T xEndPos, double& high, double& low);




};

