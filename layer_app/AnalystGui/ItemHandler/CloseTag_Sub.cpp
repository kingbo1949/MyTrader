#include "CloseTag_Sub.h"
#include <Factory_QDatabase.h>
CCloseTag_Sub::CCloseTag_Sub(QCustomPlot* parent)
	:CCloseTag(parent)
{
}
QCPAxisQPtr CCloseTag_Sub::GetX()
{
	return GetAxisRect(MainOrSub::SubT)->axis(QCPAxis::atBottom);
}

QCPAxisQPtr CCloseTag_Sub::GetY()
{
	return GetAxisRect(MainOrSub::SubT)->axis(QCPAxis::atRight);
}
void CCloseTag_Sub::DrawTag(const KlinePlotSuit& klinePlotSuit)
{
	if (klinePlotSuit.klines.empty()) return;

	double showNum = GetShowNumber(klinePlotSuit.codeId, klinePlotSuit.timeType, klinePlotSuit.klines.back()->time);
	MakeAndGet_AxisTag()->updatePosition(showNum);
	MakeAndGet_AxisTag()->setText(QString::number(showNum, 'f', 2));

	if (Get_SubVisible() == SubVisible::Show)
	{
		MakeAndGet_AxisTag()->setVisible(true);
	}else
	{
		MakeAndGet_AxisTag()->setVisible(false);
	}

	return;
}

double CCloseTag_Sub::GetShowNumber(const CodeStr &codeId, Time_Type timeType, Tick_T lastTime)
{
	QQuery query;
	query.query_type = QQueryType::FROM_CURRENT;
	query.query_number = 1;

	double ret = 0.0;
	if (Get_SubType() == SubType::Macd)
	{
		IBMacdPtrs macds = MakeAndGet_QDatabase()->GetMacds(codeId, timeType, query);
		if (macds.empty()) return ret;
		if (macds.back()->time != lastTime) return ret;
		ret = macds.back()->dif;

	}
	if (Get_SubType() == SubType::Atr)
	{
		IBAtrPtrs atrs = MakeAndGet_QDatabase()->GetAtrs(codeId, timeType, query);
		if (atrs.empty()) return ret;
		if (atrs.back()->time != lastTime) return ret;
		ret = atrs.back()->avgAtr;

	}
	return ret;

}
