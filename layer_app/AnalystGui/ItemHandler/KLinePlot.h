#pragma once
#include "ItemsHandler.h"
class CKLinePlot : public CItemsHandler
{
	Q_OBJECT

public:
	CKLinePlot(QCustomPlot* parent, XTickerQSPtr textTicker);
	virtual ~CKLinePlot() { ; };

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

	// 得到坐标系
	virtual QCPAxisQPtr		GetX() override;
	virtual QCPAxisQPtr		GetY() override;

protected:
	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override final { return true; };

private:
	QCPFinancialQPtr		m_candlesticks;
	XTickerQSPtr 			m_textTicker;

	void					InitAxis();
	void					InitCandlesticks();

	// 添加横轴刻度
	void					AddXTicker(IBKLinePtr kline, int index);
	// 删除横轴刻度上最后几个脏数据
	void					DelXTicker(int count);

	// 添加纵轴数据
	void					AddYData(IBKLinePtr kline, int index);

	QCPFinancialData		GetYData(const IBKLinePtr& kline, size_t pos);
	QString					GetXData(const IBKLinePtr& kline);


};

