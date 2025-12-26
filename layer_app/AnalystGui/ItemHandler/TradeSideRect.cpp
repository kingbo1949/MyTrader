#include "TradeSideRect.h"
#include <Factory_QDatabase.h>
CTradeSideRect::CTradeSideRect(QCustomPlot* parent)
	:CItemsHandler(parent)
{
}

void CTradeSideRect::AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit)
{
	// 坐标改变 调整m_rects所有元素的纵坐标
	for (auto item : m_rects)
	{
		ModifyYCoords(item);
	}
}

void CTradeSideRect::SetKLines(const KlinePlotSuit& klinePlotSuit)
{
	//TestDrawRect();

	ClearItems();
	if (klinePlotSuit.timeType == Time_Type::D1 || klinePlotSuit.timeType == Time_Type::H1) return;

	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(klinePlotSuit.codeId);
	if (!contract) return;

	TSideRanges ranges = ScanKlines(klinePlotSuit.codeId, klinePlotSuit.klines);
	for (const auto& range : ranges)
	{
		DrawRange(contract->securityType, range);
	}
	return;

}

QCPItemRectQPtr CTradeSideRect::MakeOneRect()
{
	// 定义矩形， 把左上角和右小角与指定坐标系绑定 但此时并没有确定该矩形具体的坐标范围

	QCPItemRectQPtr rect = new QCPItemRect(m_customPlot);
	rect->topLeft->setAxes(GetX(), GetY());
	rect->bottomRight->setAxes(GetX(), GetY());
	//rect->setLayer("background");
	//rect->setLayer("overlay");


	//if (!m_customPlot->layer("backgroundRect")) 
	//{
	//	m_customPlot->addLayer("backgroundRect", m_customPlot->layer("main"), QCustomPlot::limAbove);
	//}
	//rect->setLayer("backgroundRect");
	m_rects.push_back(rect);


	return rect;
}

void CTradeSideRect::ClearItems()
{
	for (auto oneItem : m_rects)
	{
		m_customPlot->removeItem(oneItem);
	}
	m_rects.clear();

}

TSideRanges CTradeSideRect::ScanKlines(const CodeStr& codeId, const IBKLinePtrs& klines)
{
	TSideRanges ret;
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeId);
	if (!contract) return ret;

	ret = MakeAndGet_TSideTimeHandler(contract->securityType)->ScanKlines(klines);
	return ret;
}

void CTradeSideRect::DrawRange(SecurityType securityType, const TSideRange& range)
{
	if (range.tSideType == TradeSideType::TradeIn) return;

	QCPItemRectQPtr rect = MakeOneRect();
	QColor rectColor = MakeAndGet_TSideTimeHandler(securityType)->GetTSideColor(range.tSideType);
	rect->setBrush(QBrush(rectColor));
	rect->setPen(Qt::NoPen);

	// 矩形左边界
	double left = range.beginIndex - GetKlineWidth() / 2;
	// 矩形右边界
	double right = range.endIndex + 1 - GetKlineWidth() / 2;

	ModifyXCoords(rect, left, right);
	ModifyYCoords(rect);

	return;
}

void CTradeSideRect::ModifyXCoords(QCPItemRectQPtr rect, double left, double right)
{
	QPointF topLeft = rect->topLeft->coords();
	QPointF bottomRight = rect->bottomRight->coords();
	topLeft.setX(left);
	bottomRight.setX(right);

	rect->topLeft->setCoords(topLeft);
	rect->bottomRight->setCoords(bottomRight);
}

void CTradeSideRect::ModifyYCoords(QCPItemRectQPtr rect)
{
	QPointF topLeft = rect->topLeft->coords();
	QPointF bottomRight = rect->bottomRight->coords();

	QCPRange yRange = GetY()->range();
	topLeft.setY(yRange.upper);
	bottomRight.setY(yRange.lower);

	rect->topLeft->setCoords(topLeft);
	rect->bottomRight->setCoords(bottomRight);


}

void CTradeSideRect::TestDrawRect()
{
	QCPItemRect* rect = new QCPItemRect(m_customPlot);
	rect->topLeft->setAxes(GetX(), GetY()); // 使用主图的 yAxis2
	rect->bottomRight->setAxes(GetX(), GetY()); // 使用主图的 yAxis2
	//rect->setLayer("background");
	//GetY()->rescale();

	double yMin = GetY()->range().lower;
	double yMax = GetY()->range().upper;

	rect->topLeft->setCoords(QPointF(0, yMax));
	rect->bottomRight->setCoords(QPointF(20, yMin));
	rect->setBrush(QBrush(QColor(255, 0, 0, 50)));
}

QCPAxisQPtr CTradeSideRect::GetX()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atBottom);

}

QCPAxisQPtr CTradeSideRect::GetY()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atRight);
}
