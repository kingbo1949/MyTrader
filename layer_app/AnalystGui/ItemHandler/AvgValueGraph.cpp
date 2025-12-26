#include "AvgValueGraph.h"
#include <Global.h>
CAvgValueGraph::CAvgValueGraph(QCustomPlot* parent)
	:CItemsHandler(parent)
{
	m_graph5 = m_customPlot->addGraph(GetX(), GetY());
	m_graph5->setPen(QPen(MakeAndGet_ColorManager()->GetColor().line5));

	m_graph20 = m_customPlot->addGraph(GetX(), GetY());
	m_graph20->setPen(QPen(MakeAndGet_ColorManager()->GetColor().line20));

	m_graph60 = m_customPlot->addGraph(GetX(), GetY());
	m_graph60->setPen(QPen(MakeAndGet_ColorManager()->GetColor().line60));

	m_graph200 = m_customPlot->addGraph(GetX(), GetY());
	m_graph200->setPen(QPen(MakeAndGet_ColorManager()->GetColor().line200));

	m_graph5->setLayer("axes");
	m_graph20->setLayer("axes");
	m_graph60->setLayer("axes");
	m_graph200->setLayer("axes");
}

void CAvgValueGraph::SetKLines(const KlinePlotSuit& klinePlotSuit)
{
	if (klinePlotSuit.klines.empty()) return;

	m_mapAvgValue.clear();

	while(!MakeMap(klinePlotSuit.codeId, klinePlotSuit.timeType, klinePlotSuit.klines, 0))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	;

	SetKLines(klinePlotSuit.timeType, klinePlotSuit.klines, 5);
	SetKLines(klinePlotSuit.timeType, klinePlotSuit.klines, 20);
	SetKLines(klinePlotSuit.timeType, klinePlotSuit.klines, 60);
	SetKLines(klinePlotSuit.timeType, klinePlotSuit.klines, 200);

}

void CAvgValueGraph::UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount)
{
	int addcount = klineChgCount.addcount;
	int chgcount = klineChgCount.chgcount;

	int beginPos = int(klinePlotSuit.klines.size()) - (chgcount + addcount);
	while (!MakeMap(klinePlotSuit.codeId, klinePlotSuit.timeType, klinePlotSuit.klines, beginPos))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}


	// 本回合数据
	UpdateKlines(klinePlotSuit.timeType, klinePlotSuit.klines, 5, addcount, chgcount);
	UpdateKlines(klinePlotSuit.timeType, klinePlotSuit.klines, 20, addcount, chgcount);
	UpdateKlines(klinePlotSuit.timeType, klinePlotSuit.klines, 60, addcount, chgcount);
	UpdateKlines(klinePlotSuit.timeType, klinePlotSuit.klines, 200, addcount, chgcount);

}

QCPAxisQPtr CAvgValueGraph::GetX()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atBottom);

}

QCPAxisQPtr CAvgValueGraph::GetY()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atRight);
}

void CAvgValueGraph::SetKLines(Time_Type timeType, const IBKLinePtrs& klines, int maCircle)
{
	QCPGraphQPtr pMaGraph = GetGraphLine(maCircle);
	pMaGraph->data()->clear();

	for (auto i = 0; i < klines.size(); ++i)
	{
		auto pos = m_mapAvgValue.find(klines[i]->time);
		if (pos == m_mapAvgValue.end()) continue;

		double& v = GetCircleValue(pos->second, maCircle);
		pMaGraph->addData(i, v);
	}

}

void CAvgValueGraph::UpdateKlines(Time_Type timeType, const IBKLinePtrs& klines, int maCircle, int addcount, int chgcount)
{
	// 本回合数据
	int beginPos = int(klines.size()) - (chgcount + addcount);

	// 删除chgcount个脏数据
	DelCurveData(maCircle, chgcount);

	for (auto i = beginPos; i < klines.size(); ++i)
	{
		auto pos = m_mapAvgValue.find(klines[i]->time);
		if (pos == m_mapAvgValue.end())
		{
			qDebug() << "can not find:" << CGlobal::GetTickTimeStr(klines[i]->time).c_str();
			continue;
		}

		double& v = GetCircleValue(pos->second, maCircle);
		GetGraphLine(maCircle)->addData(i, v);
	}

}

QCPGraphQPtr CAvgValueGraph::GetGraphLine(int circle)
{
	if (circle == 5) return m_graph5;
	if (circle == 20) return m_graph20;
	if (circle == 60) return m_graph60;
	if (circle == 200) return m_graph200;

	return nullptr;
}


void CAvgValueGraph::DelCurveData(int circle, int count)
{
	auto dataContainer = GetGraphLine(circle)->data();
	if (count == 0 || dataContainer->size() < count) return;

	int beginPos = int(dataContainer->size()) - count;
	dataContainer->removeAfter(beginPos - 0.000001);

	return;

}
