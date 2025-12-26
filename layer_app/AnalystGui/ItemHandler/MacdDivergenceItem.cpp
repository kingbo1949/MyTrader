#include "MacdDivergenceItem.h"
#include <algorithm>
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
CMacdDivergenceItem::CMacdDivergenceItem(QCustomPlot* parent)
	:CItemsHandler(parent)
{
}
void CMacdDivergenceItem::AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit)
{
	// 坐标变化需要移动顶部箭头
	QCPRange yRange = GetY()->range();
	// 箭头长度
	double arrowLenth = (yRange.upper - yRange.lower) * (GetYAxisFactor() - 1) / 3;
	double upper_top = yRange.upper;
	double lower_top = yRange.upper - arrowLenth;

	double upper_bottom = yRange.lower + arrowLenth;
	double lower_bottom = yRange.lower;

	for (auto item : m_arrows)
	{
		QCPItemLineQPtr arrow = item.second.arrow;
		QPointF startPoint = arrow->start->coords();
		QPointF endPoint = arrow->end->coords();

		if (item.second.type == DivergenceType::Top || item.second.type == DivergenceType::TopSub)
		{
			MovePoints(startPoint, endPoint, upper_top, lower_top);
		}
		else
		{
			MovePoints(startPoint, endPoint, upper_bottom, lower_bottom);
		}

		arrow->start->setCoords(startPoint);
		arrow->end->setCoords(endPoint);
	}



}
void CMacdDivergenceItem::SetKLines(const KlinePlotSuit& klinePlotSuit)
{
	ClearItems();

	m_mapDivTypeValue.clear();
	while (!MakeMap(klinePlotSuit.codeId, klinePlotSuit.timeType, klinePlotSuit.klines, 0))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}


	for (int i = 0; i < klinePlotSuit.klines.size(); ++i)
	{
		auto pos = m_mapDivTypeValue.find(klinePlotSuit.klines[i]->time);
		if (pos == m_mapDivTypeValue.end()) continue;

		if (pos->second->divType == DivergenceType::Normal) continue;

		DivergenceArrow divergenceArrow = MakeOneArrow(i, pos->second);
		SetupArrowAtIndex(i, divergenceArrow);
	}


}
void CMacdDivergenceItem::UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount)
{
	// 本回合数据
	int beginPos = int(klinePlotSuit.klines.size()) - (klineChgCount.chgcount + klineChgCount.addcount);
	while (!MakeMap(klinePlotSuit.codeId, klinePlotSuit.timeType, klinePlotSuit.klines, beginPos))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	;

	UpdateKlines(klinePlotSuit.timeType, klinePlotSuit.klines, klineChgCount.addcount, klineChgCount.chgcount);

	return;


}
QCPAxisQPtr CMacdDivergenceItem::GetX()
{
	return GetAxisRect(MainOrSub::SubT)->axis(QCPAxis::atBottom);

}

QCPAxisQPtr CMacdDivergenceItem::GetY()
{
	return GetAxisRect(MainOrSub::SubT)->axis(QCPAxis::atRight);
}

bool CMacdDivergenceItem::MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos)
{
	if (klines.empty()) return false;

	QQuery query;
	query.query_type = QQueryType::BETWEEN_TIME;
	query.time_pair.beginPos = klines[beginPos]->time;
	query.time_pair.endPos = klines.back()->time + 10;

	IBDivTypePtrs divTypeValues = MakeAndGet_QDatabase()->GetDivTypes(codeId, timetype, query);
	if (divTypeValues.empty()) return false;
	if (divTypeValues.back()->time != klines.back()->time) return false;
	if (divTypeValues[0]->time != klines[beginPos]->time) return false;

	for (auto value : divTypeValues)
	{
		m_mapDivTypeValue[value->time] = value;
	}
	return true;

}

void CMacdDivergenceItem::ClearItems()
{
	for (auto oneItem : m_arrows)
	{
		m_customPlot->removeItem(oneItem.second.arrow);
	}
	m_arrows.clear();

}

void CMacdDivergenceItem::DelDataAtIndex(int index)
{
	auto item = m_arrows.find(index);
	if (item == m_arrows.end()) return;

	m_customPlot->removeItem(item->second.arrow);
	m_arrows.erase(item);
	return;
}

DivergenceArrow CMacdDivergenceItem::MakeOneArrow(int index, IBDivTypePtr type)
{
	DivergenceArrow ret;

	QCPItemLineQPtr arrow = new QCPItemLine(m_customPlot);
	arrow->setHead(QCPLineEnding::esSpikeArrow); // 设置箭头类型
	if (type->divType == DivergenceType::Top || type->divType == DivergenceType::Bottom)
	{
		arrow->setHead(QCPLineEnding::esSpikeArrow); // 设置箭头类型
	}
	if (type->divType == DivergenceType::TopSub || type->divType == DivergenceType::BottomSub)
	{
		arrow->setHead(QCPLineEnding::esDiamond); // 设置箭头类型
	}

	arrow->setLayer("overlay");
	arrow->start->setAxes(GetX(), GetY());
	arrow->end->setAxes(GetX(), GetY());
	arrow->setClipAxisRect(nullptr);

	ret.type = type->divType;
	ret.isUTurn = type->isUTurn;
	ret.arrow = arrow;


	m_arrows[index] = ret;
	return ret;
}

void CMacdDivergenceItem::SetupArrowAtIndex(int index, DivergenceArrow divergenceArrow)
{
	QCPRange yRange = GetY()->range();
	// 箭头长度
	double arrowLenth = (yRange.upper - yRange.lower) * (GetYAxisFactor() - 1) / 3;

	QPointF beginPoint(index, 0);;
	QPointF endPoint(index, 0);
	if (divergenceArrow.type == DivergenceType::Top || divergenceArrow.type == DivergenceType::TopSub)
	{
		beginPoint.setY(yRange.upper);
		endPoint.setY(yRange.upper - arrowLenth);
	}
	else
	{
		beginPoint.setY(yRange.lower);
		endPoint.setY(yRange.lower + arrowLenth);
	}
	divergenceArrow.arrow->start->setCoords(beginPoint);
	divergenceArrow.arrow->end->setCoords(endPoint);

	if (divergenceArrow.type == DivergenceType::Top)
	{
		divergenceArrow.arrow->setPen(QPen(MakeAndGet_ColorManager()->GetColor().arrowBuy, 2, Qt::SolidLine));
	}
	else if (divergenceArrow.type == DivergenceType::Bottom)
	{
		divergenceArrow.arrow->setPen(QPen(MakeAndGet_ColorManager()->GetColor().arrowSell, 2, Qt::SolidLine));
	}
	else
	{
		divergenceArrow.arrow->setPen(QPen(MakeAndGet_ColorManager()->GetColor().lineLoss, 2, Qt::SolidLine));
	}

	if (divergenceArrow.isUTurn)
	{
		divergenceArrow.arrow->setPen(QPen(MakeAndGet_ColorManager()->GetColor().lineProfit, 2, Qt::SolidLine));
	}

	return;
}

void CMacdDivergenceItem::UpdateKlines(Time_Type timeType, const IBKLinePtrs& klines, int addcount, int chgcount)
{
	// 本回合数据
	int beginPos = int(klines.size()) - (chgcount + addcount);
	int beginPosChg = beginPos;
	int beginPosAdd = int(klines.size()) - addcount;

	// 删除脏数据
	for (int i = beginPos; i < klines.size(); ++i)
	{
		DelDataAtIndex(i);
	}

	for (auto i = beginPos; i < klines.size(); ++i)
	{
		auto pos = m_mapDivTypeValue.find(klines[i]->time);
		if (pos == m_mapDivTypeValue.end()) continue;

		if (pos->second->divType == DivergenceType::Normal) continue;

		DivergenceArrow divergenceArrow = MakeOneArrow(i, pos->second);
		SetupArrowAtIndex(i, divergenceArrow);
	}

}





