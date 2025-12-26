#pragma once
#include "../qcustomplot/qcustomplot.h"
class CAxisTickerText_KLine : public QCPAxisTickerText
{
protected:

	// 自定义刻度
	virtual QVector<double> createTickVector(double tickStep, const QCPRange& range) override;

};

