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

	QQuery query;
	query.query_type = QQueryType::FROM_CURRENT;
	query.query_number = 1;
	IBMacdPtrs macds = MakeAndGet_QDatabase()->GetMacds(klinePlotSuit.codeId, klinePlotSuit.timeType, query);
	if (macds.empty()) return;
	if (macds.back()->time != klinePlotSuit.klines.back()->time) return;


	double macd_dif = macds.back()->dif;
	MakeAndGet_AxisTag()->updatePosition(macd_dif);
	MakeAndGet_AxisTag()->setText(QString::number(macd_dif, 'f', 2));
	return;
}
