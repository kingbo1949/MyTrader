#include "MacdGraph.h"
#include "../src/AxisTickerText_KLine.h"
#include <Global.h>
#include <QtGlobal.h>
#include <Factory_QDatabase.h>
CMacdGraph::CMacdGraph(QCustomPlot* parent, XTickerQSPtr textTicker)
	:CItemsHandler(parent), m_textTicker(textTicker)
{
	InitAxis();

	m_macd = m_customPlot->addGraph(GetX(), GetY());
	m_macd->setPen(QColor(41, 98, 255));


	m_dea = m_customPlot->addGraph(GetX(), GetY());
	m_dea->setPen(QColor(255, 109, 0));

	m_macdBarPositive = new QCPBars(GetX(), GetY());
	m_macdBarPositive->setPen(Qt::NoPen);
	m_macdBarPositive->setBrush(QBrush(QColor(38, 166, 154)));

	m_macdBarNegative = new QCPBars(GetX(), GetY());
	m_macdBarNegative->setPen(Qt::NoPen);
	m_macdBarNegative->setBrush(QBrush(QColor(255, 82, 82)));


	m_macd->setLayer("axes");
	m_dea->setLayer("axes");
	m_macdBarPositive->setLayer("axes");
	m_macdBarNegative->setLayer("axes");
}

void CMacdGraph::AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit)
{
	double high, low;
	int xBeginPos, xEndPos;
	GetWindowsPos(xBeginPos, xEndPos);
	if (xBeginPos < 0) xBeginPos = 0;
	if (xEndPos >= klinePlotSuit.klines.size()) xEndPos = int(klinePlotSuit.klines.size() - 1);
	GetHighLow(klinePlotSuit.klines[xBeginPos]->time, klinePlotSuit.klines[xEndPos]->time + 10, high, low);
	QCPRange yRange(low, high);
	GetY()->setRange(yRange);
	GetY()->scaleRange(GetYAxisFactor(), GetY()->range().center());

	return;

}

void CMacdGraph::SetKLines(const KlinePlotSuit& klinePlotSuit)
{
	if (klinePlotSuit.klines.empty()) return;

	m_macd->data()->clear();
	m_dea->data()->clear();
	m_macdBarPositive->data()->clear();
	m_macdBarNegative->data()->clear();

	m_mapMacdValue.clear();

	while (!MakeMap(klinePlotSuit.codeId, klinePlotSuit.timeType, klinePlotSuit.klines, 0))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}




	for (auto i = 0; i < klinePlotSuit.klines.size(); ++i)
	{
		auto pos = m_mapMacdValue.find(klinePlotSuit.klines[i]->time);
		if (pos == m_mapMacdValue.end()) continue;

		m_macd->addData(i, pos->second->dif);
		m_dea->addData(i, pos->second->dea);

		if (pos->second->macd > 0)
		{
			m_macdBarPositive->addData(i, pos->second->macd);
		}
		else
		{
			m_macdBarNegative->addData(i, pos->second->macd);
		}
	}
	m_macdBarPositive->setWidth(GetKlineWidth());
	m_macdBarNegative->setWidth(GetKlineWidth());

	// 修改坐标范围
	// 因为有可能是切换品种，而其他元素绘图的范围无法预知，因此不能使用rescaleAxes来自动调整坐标
	// x轴由主图修改，这里不需要修改
	double high, low;
	int xBeginPos, xEndPos;
	GetWindowsPos(xBeginPos, xEndPos);
	if (xBeginPos < 0) xBeginPos = 0;
	if (xEndPos >= int(klinePlotSuit.klines.size())) xEndPos = int(klinePlotSuit.klines.size() - 1);

	GetHighLow(klinePlotSuit.klines[xBeginPos]->time, klinePlotSuit.klines[xEndPos]->time + 10, high, low);
	QCPRange yRange(low, high);
	GetY()->setRange(yRange);
	GetY()->scaleRange(GetYAxisFactor(), GetY()->range().center());


}

void CMacdGraph::UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount)
{
	int addcount = klineChgCount.addcount;
	int chgcount = klineChgCount.chgcount;

	// 本回合数据
	int beginPos = int(klinePlotSuit.klines.size()) - (chgcount + addcount);
	while (!MakeMap(klinePlotSuit.codeId, klinePlotSuit.timeType, klinePlotSuit.klines, beginPos))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	int beginPosChg = beginPos;
	int beginPosAdd = int(klinePlotSuit.klines.size()) - addcount;

	// 删除脏数据
	DelData(beginPos, m_macd);
	DelData(beginPos, m_dea);
	DelData(beginPos, m_macdBarPositive);
	DelData(beginPos, m_macdBarNegative);

	for (auto i = beginPos; i < klinePlotSuit.klines.size(); ++i)
	{
		auto pos = m_mapMacdValue.find(klinePlotSuit.klines[i]->time);
		if (pos == m_mapMacdValue.end()) continue;

		m_macd->addData(i, pos->second->dif);
		m_dea->addData(i, pos->second->dea);
		if (pos->second->macd > 0)
		{
			m_macdBarPositive->addData(i, pos->second->macd);
		}
		else
		{
			m_macdBarNegative->addData(i, pos->second->macd);
		}
	}

	// 判断新数据是否产生了高低点，而需要改变纵坐标容纳新数据
	double high, low;
	GetHighLow(klinePlotSuit.klines[beginPos]->time, klinePlotSuit.klines.back()->time + 10, high, low);
	QCPRange yRange = GetY()->range();
	if (high > yRange.upper || low < yRange.lower)
	{
		GetY()->scaleRange(GetYAxisFactor(), GetY()->range().center());
	}
	return;


}

QCPAxisQPtr CMacdGraph::GetX()
{
	return GetAxisRect(MainOrSub::SubT)->axis(QCPAxis::atBottom);

}

QCPAxisQPtr CMacdGraph::GetY()
{
	return GetAxisRect(MainOrSub::SubT)->axis(QCPAxis::atRight);
}

bool CMacdGraph::MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos)
{
	if (klines.empty()) return false;

	QQuery query;
	query.query_type = QQueryType::BETWEEN_TIME;
	query.time_pair.beginPos = klines[beginPos]->time;
	query.time_pair.endPos = klines.back()->time + 10;

	IBMacdPtrs macdValues = MakeAndGet_QDatabase()->GetMacds(codeId, timetype, query);
	if (macdValues.empty()) return false;
	if (macdValues.back()->time != klines.back()->time) return false;
	if (macdValues[0]->time != klines[beginPos]->time) return false;


	for (auto value : macdValues)
	{
		m_mapMacdValue[value->time] = value;
	}
	return true;

}

void CMacdGraph::InitAxis()
{
	// 用自定义类来设置横轴刻度
	GetX()->setTicker(m_textTicker);

	// 设置刻度的值的数量，如果数量太多，轴的刻度会显示的密密麻麻不美观
	m_textTicker->setTickCount(10);
	GetY()->ticker()->setTickCount(5);

}


void CMacdGraph::GetWindowsPos(int& beginPos, int& endPos)
{
	QCPRange xRange = GetX()->range();

	beginPos = std::floor(xRange.lower);
	endPos = std::ceil(xRange.upper);
}

void CMacdGraph::GetHighLow(Tick_T xBeginPos, Tick_T xEndPos, double& high, double& low)
{
	high = -99999999.9;
	low = 99999999.9;

	// 使用 lower_bound 找到键 >= xBeginPos 的第一个元素
	auto it = m_mapMacdValue.lower_bound(xBeginPos);

	// 遍历从 it 到末尾的所有项
	for (; it != m_mapMacdValue.end(); ++it)
	{
		if (it->second->time >= xEndPos) return;

		if (std::isnan(it->second->dif))
		{
			continue;
		}

		if (it->second->dif > high) high = it->second->dif;
		if (it->second->dif < low) low = it->second->dif;
	}
	return;

}

