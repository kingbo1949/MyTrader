#include "ItemsHandler.h"

CItemsHandler::CItemsHandler(QCustomPlot* parent)
	:CPlotObject(parent)
{
}

void CItemsHandler::PrintRange(QCustomPlot* customPlot)
{
	QCPRange xRange = customPlot->xAxis->range();
	QCPRange yRange = customPlot->yAxis2->range();
	//qDebug() << "X轴范围: " << xRange.lower << " 到 " << xRange.upper;
	//qDebug() << "Y轴范围: " << yRange.lower << " 到 " << yRange.upper;

}

void CItemsHandler::MovePoints(QPointF& point1, QPointF& point2, double upper, double lower)
{
	if (point1.y() > point2.y())
	{
		// point1为高点
		point1.setY(upper);
		point2.setY(lower);
	}
	else
	{
		// point2为高点
		point2.setY(upper);
		point1.setY(lower);
	}
	return;

}
