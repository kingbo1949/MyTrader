#include "TMatchesItems.h"
#include <Factory_IBGlobalShare.h>
#include "../src/BracketWithArrow.h"
#include "../src/Factory_AnalystGui.h"
CTMatchesItems::CTMatchesItems(QCustomPlot* parent)
	:CItemsHandler(parent)
{
}





void CTMatchesItems::AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit)
{
	// 坐标变化需要移动顶部箭头
	QCPRange yRange = GetY()->range();
	// 箭头长度
	double arrowLenth = (yRange.upper - yRange.lower) * (GetYAxisFactor() - 1) / 3;
	double upper = yRange.upper;
	double lower = yRange.upper - arrowLenth;

	for (auto arrow : m_topArrows)
	{
		QPointF startPoint = arrow->start->coords();
		QPointF endPoint = arrow->end->coords();
		MovePoints(startPoint, endPoint, upper, lower);
		arrow->start->setCoords(startPoint);
		arrow->end->setCoords(endPoint);
	}
}


void CTMatchesItems::SetMatches(const KlinePlotSuit& klinePlotSuit)
{
	ClearItems();
	for (const auto& oneMatch : klinePlotSuit.tMatches)
	{
		TMatchPointPairPtr  pTMatchPair = MakeTMatchPair(klinePlotSuit.timeType, klinePlotSuit.klines, oneMatch);

		if (!pTMatchPair)
		{
			Get_LogFunc()(LogLevel::Err, "tMatchPointPair is null");
			continue;
		}

		DrawOneMatch(pTMatchPair);
	}
}

TMatchPointPairPtr CTMatchesItems::MakeTMatchPair(Time_Type timeType, const IBKLinePtrs klines, SimpleMatchPtr oneMatch)
{
	TMatchPointPairPtr ret = std::make_shared<TMatchPointPair>();
	ret->open_buyOrSell = oneMatch->strategyKey.buyOrSell;

	for (auto i = 0; i < klines.size(); ++i)
	{
		if (!CGetRecordNo::TimeInKLine(timeType, klines[i], oneMatch->dealtime_open)) continue;

		ret->beginPoint.setX(i);;
		ret->beginPoint.setY(oneMatch->price_open);
		break;
	}
	if (qRound(ret->beginPoint.x()) < 0) return nullptr;

	for (auto j = qRound(ret->beginPoint.x()); j < klines.size(); ++j)
	{
		if (!CGetRecordNo::TimeInKLine(timeType, klines[j], oneMatch->dealtime_cover)) continue;

		ret->endPoint.setX(j);
		ret->endPoint.setY(oneMatch->price_cover);
		if (oneMatch->profit > 0)
		{
			ret->haveProfit = true;
		}
		else
		{
			ret->haveProfit = false;
		}
		return ret;
	}

	return nullptr;

}

void CTMatchesItems::DrawOneMatch(TMatchPointPairPtr tMatchPointPair)
{
	if (qRound(tMatchPointPair->beginPoint.x()) == qRound(tMatchPointPair->endPoint.x()))
	{
		DrawOneMatch_Same(tMatchPointPair);
	}
	else
	{
		DrawOneMatch_Diff(tMatchPointPair);
	}
}

void CTMatchesItems::DrawOneMatch_Same(TMatchPointPairPtr tMatchPointPair)
{
	CBracketWithArrow* bracket = MakeOneBracketWithArrow();
	bracket->left->setCoords(ModifyPoint(tMatchPointPair->beginPoint, LeftOrRight::Right));
	bracket->right->setCoords(ModifyPoint(tMatchPointPair->endPoint, LeftOrRight::Right));

	if (tMatchPointPair->haveProfit)
	{
		bracket->setPen(QPen(MakeAndGet_ColorManager()->GetColor().lineProfit, 2, Qt::DashDotDotLine));
	}
	else
	{
		bracket->setPen(QPen(MakeAndGet_ColorManager()->GetColor().lineLoss, 2, Qt::DashDotDotLine));
	}
	DrawTopArrow(tMatchPointPair->open_buyOrSell, tMatchPointPair->beginPoint.x(), LeftOrRight::Left);

	BuyOrSell cover_buyOrSell = CReverse::Reverse_BuyOrSell(tMatchPointPair->open_buyOrSell);
	DrawTopArrow(cover_buyOrSell, tMatchPointPair->endPoint.x(), LeftOrRight::Right);
	return;

}

void CTMatchesItems::DrawOneMatch_Diff(TMatchPointPairPtr tMatchPointPair)
{
	// 画回合起点到终点箭头
	QCPItemLine* arrow = MakeOneMatchArrow();
	arrow->start->setCoords(ModifyPoint(tMatchPointPair->beginPoint, LeftOrRight::Right));
	arrow->end->setCoords(ModifyPoint(tMatchPointPair->endPoint, LeftOrRight::Left));

	if (tMatchPointPair->haveProfit)
	{
		arrow->setPen(QPen(MakeAndGet_ColorManager()->GetColor().lineProfit, 2, Qt::DashDotDotLine));
	}
	else
	{
		arrow->setPen(QPen(MakeAndGet_ColorManager()->GetColor().lineLoss, 2, Qt::DashDotDotLine));
	}

	// 画顶部箭头
	DrawTopArrow(tMatchPointPair->open_buyOrSell, tMatchPointPair->beginPoint.x(), LeftOrRight::NoMove);
	BuyOrSell cover_buyOrSell = CReverse::Reverse_BuyOrSell(tMatchPointPair->open_buyOrSell);
	DrawTopArrow(cover_buyOrSell, tMatchPointPair->endPoint.x(), LeftOrRight::NoMove);

	return;

}

void CTMatchesItems::DrawTopArrow(BuyOrSell buyOrSell, double x, LeftOrRight leftOrRight)
{
	QCPRange yRange = GetY()->range();
	// 箭头长度
	double arrowLenth = (yRange.upper - yRange.lower) * (GetYAxisFactor() - 1) / 3;

	QCPItemLine* arrow = MakeOneTopArrow();

	QPointF beginPoint(x, 0);;
	QPointF endPoint(x, 0);
	if (buyOrSell == BuyOrSell::Buy)
	{
		beginPoint.setY(yRange.upper - arrowLenth);
		endPoint.setY(yRange.upper);
	}
	else
	{
		beginPoint.setY(yRange.upper);
		endPoint.setY(yRange.upper - arrowLenth);
	}
	beginPoint = ModifyPoint(beginPoint, leftOrRight);
	endPoint = ModifyPoint(endPoint, leftOrRight);

	arrow->start->setCoords(beginPoint);
	arrow->end->setCoords(endPoint);

	if (buyOrSell == BuyOrSell::Buy)
	{
		arrow->setPen(QPen(MakeAndGet_ColorManager()->GetColor().arrowBuy, 2, Qt::SolidLine));
	}
	else
	{
		arrow->setPen(QPen(MakeAndGet_ColorManager()->GetColor().arrowSell, 2, Qt::SolidLine));
	}
	return;
}

QPointF CTMatchesItems::ModifyPoint(QPointF point, LeftOrRight leftOrRight)
{
	QPointF ret = point;
	if (leftOrRight == LeftOrRight::NoMove) return ret;

	// Y坐标不变，仅调整X坐标
	if (leftOrRight == LeftOrRight::Left)
	{
		ret.setX(point.x() - GetKlineWidth() / 2);
	}
	else
	{
		ret.setX(point.x() + GetKlineWidth() / 2);
	}
	return ret;


}

QCPItemLineQPtr CTMatchesItems::MakeOneTopArrow()
{
	QCPItemLineQPtr arrow = new QCPItemLine(m_customPlot);
	arrow->setHead(QCPLineEnding::esSpikeArrow); // 设置箭头类型
	arrow->setLayer("overlay");
	arrow->start->setAxes(GetX(), GetY());
	arrow->end->setAxes(GetX(), GetY());

	m_topArrows.push_back(arrow);

	return arrow;
}

BracketWithArrowQPtr CTMatchesItems::MakeOneBracketWithArrow()
{
	BracketWithArrowQPtr bracket = new CBracketWithArrow(m_customPlot);
	bracket->setLength(10); // 中括号拱起的方向与长度
	bracket->setArrow(false, true);
	bracket->setLayer("overlay");
	bracket->left->setAxes(GetX(), GetY());
	bracket->right->setAxes(GetX(), GetY());

	TwoArrow twoArrow = bracket->GetTwoArrow();
	twoArrow.arrow_left->setLayer("overlay");
	twoArrow.arrow_left->start->setAxes(GetX(), GetY());
	twoArrow.arrow_left->end->setAxes(GetX(), GetY());
	twoArrow.arrow_right->setLayer("overlay");
	twoArrow.arrow_right->start->setAxes(GetX(), GetY());
	twoArrow.arrow_right->end->setAxes(GetX(), GetY());

	m_matchArrows.push_back(bracket);
	m_matchArrows.push_back(twoArrow.arrow_left);
	m_matchArrows.push_back(twoArrow.arrow_right);

	return bracket;
}

QCPItemLineQPtr CTMatchesItems::MakeOneMatchArrow()
{
	QCPItemLineQPtr arrow = new QCPItemLine(m_customPlot);
	arrow->setHead(QCPLineEnding::esSpikeArrow); // 设置箭头类型
	arrow->setLayer("overlay");
	arrow->start->setAxes(GetX(), GetY());
	arrow->end->setAxes(GetX(), GetY());

	m_matchArrows.push_back(arrow);
	return arrow;
}

void CTMatchesItems::ClearItems()
{
	for (auto oneItem : m_topArrows)
	{
		m_customPlot->removeItem(oneItem);
	}
	m_topArrows.clear();

	for (auto oneItem : m_matchArrows)
	{
		m_customPlot->removeItem(oneItem);
	}
	m_matchArrows.clear();



}


QCPAxisQPtr CTMatchesItems::GetX()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atBottom);

}

QCPAxisQPtr CTMatchesItems::GetY()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atRight);
}
