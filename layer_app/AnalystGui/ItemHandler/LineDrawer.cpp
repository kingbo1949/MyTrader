#include "LineDrawer.h"

CLineDrawer::CLineDrawer(QCustomPlot* parent)
	:CItemsHandler(parent)
{
	// 初始化m_codeIdToLines
	// m_codeIdToLines["ES"] = { 6024, 6048, 6100 };
	m_codeIdToLines["NQ"] = { 25319, 25500, 26447};
	m_codeIdToLines["SOXX"] = { 257.5, 269.3, 272.3, 284.5, 298 };
	m_codeIdToLines["ES"] = {6818, 6900, 6960};
	m_codeIdToLines["AMD"] = { 186.6, 232};
	m_codeIdToLines["META"] = { 794.37, 800.68 };
	m_codeIdToLines["AMZN"] = { 236, 238.9 }; 
	m_codeIdToLines["NVDA"] = { 177.99, 185.4, 189.77, 192.4 };
	m_codeIdToLines["TSLA"] = { 349.7, 421,437.6, 439.6, 459.4, 469.2 };
	m_codeIdToLines["MSFT"] = { 552.2, 563.7 };
	m_codeIdToLines["CRCL"] = { 153.8 };
	m_codeIdToLines["COIN"] = { 293.6 };

}
void CLineDrawer::SetKLines(const KlinePlotSuit& klinePlotSuit)
{
	ClearLines();

	std::map<CodeStr, std::vector<double>>::const_iterator pos = m_codeIdToLines.find(klinePlotSuit.codeId);
	if (pos == m_codeIdToLines.end())
	{
		return;
	}

	// 画水平线
	for (const auto& lineValue : pos->second)
	{
		QCPItemLine* line = MakeOneLine(lineValue);
	}
	return;
	
}
QCPAxisQPtr CLineDrawer::GetX()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atBottom);

}

QCPAxisQPtr CLineDrawer::GetY()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atRight);
}

QCPItemLine* CLineDrawer::MakeOneLine(double yValue) 
{
	QCPItemLine*  line = new QCPItemLine(m_customPlot);
	line->setPen(QPen(Qt::blue, 1, Qt::DashLine)); // 蓝色，粗细 1 像素，虚线
	line->setLayer("overlay");
	
	//double y = GetY()->pixelToCoord(event->pos().y());
	
	line->start->setCoords(GetX()->range().lower, yValue);
	line->end->setCoords(GetX()->range().upper, yValue);



	//line->start->setCoords(GetX()->range().lower, GetY()->range().lower);
	//line->end->setCoords(GetX()->range().lower, GetY()->range().lower);
	line->start->setAxes(GetX(), GetY());
	line->end->setAxes(GetX(), GetY());
	line->setClipAxisRect(nullptr);

	m_lines.push_back(line);
	return line;
}

void CLineDrawer::ClearLines()
{
	for (auto line : m_lines)
	{
		m_customPlot->removeItem(line);
	}
	m_lines.clear();
	return;
}
