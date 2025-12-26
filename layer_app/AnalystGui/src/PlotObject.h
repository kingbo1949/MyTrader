#pragma once
#include <qobject.h>
#include "qcustomplot.h"


class CPlotObject : public QObject
{
	Q_OBJECT

public:
	CPlotObject(QCustomPlot* parent);
	virtual ~CPlotObject() { ; };

protected:
	QCustomPlot* m_customPlot;


};

