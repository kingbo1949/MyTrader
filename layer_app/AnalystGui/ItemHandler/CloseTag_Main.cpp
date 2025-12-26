#include "CloseTag_Main.h"

CCloseTag_Main::CCloseTag_Main(QCustomPlot* parent)
	:CCloseTag(parent)
{
}

QCPAxisQPtr CCloseTag_Main::GetX()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atBottom);

}

QCPAxisQPtr CCloseTag_Main::GetY()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atRight);
}

void CCloseTag_Main::DrawTag(const KlinePlotSuit& klinePlotSuit)
{
	if (klinePlotSuit.klines.empty()) return;

	double close = klinePlotSuit.klines.back()->close;
	MakeAndGet_AxisTag()->updatePosition(close);
	MakeAndGet_AxisTag()->setText(QString::number(close, 'f', 2));
}
