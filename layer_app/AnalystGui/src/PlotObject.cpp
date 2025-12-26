#include "PlotObject.h"

CPlotObject::CPlotObject(QCustomPlot* parent)
	:QObject(parent)
{
	m_customPlot = parent;
}

