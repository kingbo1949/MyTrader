#include "KLinePlot.h"
#include "../src/AxisTickerText_KLine.h"
#include <Factory_IBGlobalShare.h>
#include <Global.h>
#include <QtGlobal.h>
CKLinePlot::CKLinePlot(QCustomPlot* parent, XTickerQSPtr textTicker)
	:CItemsHandler(parent), m_textTicker(textTicker)
{
	InitAxis();
	InitCandlesticks();
}

void CKLinePlot::AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit)
{
	QCPRange xRange = GetX()->range();
	SetMainY(xRange, klinePlotSuit.klines);

	return;

}

void CKLinePlot::SetKLines(const KlinePlotSuit& klinePlotSuit)
{
	m_textTicker->clear();
	m_candlesticks->data()->clear();
	for (auto i = 0; i < klinePlotSuit.klines.size(); ++i)
	{
		AddXTicker(klinePlotSuit.klines[i], i);
		AddYData(klinePlotSuit.klines[i], i);
	}
	SetKlineWidth(m_candlesticks->width());

	// 修改坐标范围
	// 因为有可能是切换品种，而其他元素绘图的范围无法预知，因此不能使用rescaleAxes来自动调整坐标
	QCPRange xRange(0, klinePlotSuit.klines.size());
	GetX()->setRange(xRange);
	GetX()->scaleRange(GetXAxisFactor(), GetX()->range().center());

	SetMainY(xRange, klinePlotSuit.klines);

	return;

}

void CKLinePlot::UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount)
{
	int addcount = klineChgCount.addcount;
	int chgcount = klineChgCount.chgcount;

	// 本回合数据
	int beginPos = int(klinePlotSuit.klines.size()) - (chgcount + addcount);

	// 删除脏数据
	DelXTicker(chgcount);
	DelData(beginPos, m_candlesticks);
	// 截取本回合数据
	IBKLinePtrs thisMatch = IBKLinePtrs(klinePlotSuit.klines.end() - (chgcount + addcount), klinePlotSuit.klines.end());
	int size = int(klinePlotSuit.klines.size()) - chgcount - addcount;
	for (auto i = 0; i < thisMatch.size(); ++i)
	{
		AddXTicker(thisMatch[i], i + size);
		AddYData(thisMatch[i], i + size);
	}

	// 判断是否需要随着新数据到来，视窗往后移动
	QCPRange xRange = GetX()->range();
	if (addcount != 0)
	{
		GetX()->setRange(xRange.lower + addcount, xRange.upper + addcount);
	}
	SetMainY(xRange, klinePlotSuit.klines);

	return;


}

QCPAxisQPtr CKLinePlot::GetX()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atBottom);

}

QCPAxisQPtr CKLinePlot::GetY()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atRight);
}

void CKLinePlot::InitAxis()
{
	// 用自定义类来设置横轴刻度
	GetX()->setTicker(m_textTicker);

	// 设置刻度的值的数量，如果数量太多，轴的刻度会显示的密密麻麻不美观
	m_textTicker->setTickCount(10);
	GetY()->ticker()->setTickCount(15);
}

void CKLinePlot::InitCandlesticks()
{
	m_candlesticks = new QCPFinancial(GetX(), GetY());

	QColor BrushPositive = MakeAndGet_ColorManager()->GetColor().brushPositive;
	QColor PenPositive = MakeAndGet_ColorManager()->GetColor().penPositive;
	QColor BrushNegative = MakeAndGet_ColorManager()->GetColor().brushNegative;
	QColor PenNegative = MakeAndGet_ColorManager()->GetColor().penNegative;


	m_candlesticks->setBrushPositive(BrushPositive);
	m_candlesticks->setPenPositive(PenPositive);
	m_candlesticks->setBrushNegative(BrushNegative);
	m_candlesticks->setPenNegative(PenNegative);

	// 每条网格对应一个刻度

	GetX()->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));     // 网格线(对应刻度)画笔
	GetY()->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
	GetX()->grid()->setSubGridPen(QPen(QColor(80, 80, 80, 100), 1, Qt::DotLine)); // 子网格线(对应子刻度)画笔
	GetY()->grid()->setSubGridPen(QPen(QColor(80, 80, 80, 100), 1, Qt::DotLine));
	// 是否显示子网格线
	GetX()->grid()->setSubGridVisible(false);
	GetY()->grid()->setSubGridVisible(true);

	m_candlesticks->setLayer("axes");

	// 设置刻度为0时的网格线的画笔
	//GetX()->grid()->setZeroLinePen(QPen(Qt::red));   
	//GetY()->grid()->setZeroLinePen(QPen(Qt::red));

}

void CKLinePlot::AddXTicker(IBKLinePtr kline, int index)
{
	QString label = GetXData(kline);
	m_textTicker->addTick(index, label);
	return;

}

void CKLinePlot::DelXTicker(int count)
{
	if (count == 0) return;

	QMap<double, QString>& ticks = m_textTicker->ticks();
	if (ticks.size() < count) return;

	int beforeSize = ticks.size();

	auto it = ticks.end();
	std::advance(it, -1 * count); // 向前移动n个位置
	ticks.erase(it, ticks.end());
	//qDebug() << "DelXTicker x size::" << ticks.size() << "before size:" << beforeSize;

}

void CKLinePlot::AddYData(IBKLinePtr kline, int index)
{
	QCPFinancialData data = GetYData(kline, index);
	m_candlesticks->data()->add(data);
	return;

}

QCPFinancialData CKLinePlot::GetYData(const IBKLinePtr& kline, size_t pos)
{
	QCPFinancialData data;
	data.key = pos;
	data.open = kline->open;
	data.close = kline->close;
	data.low = kline->low;
	data.high = kline->high;
	return data;
}

QString CKLinePlot::GetXData(const IBKLinePtr& kline)
{
	std::string timestr = CGlobal::GetTickTimeStr(kline->time);
	// 20231011 12:12:00.0 截取时分
	timestr = timestr.substr(9, 5);
	return CQtGlobal::StdString_QString(timestr);

}

