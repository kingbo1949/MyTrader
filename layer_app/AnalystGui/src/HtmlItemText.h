#pragma once
#include "../qcustomplot/qcustomplot.h"
class CHtmlItemText : public QCPItemText
{
	Q_OBJECT
public:
	CHtmlItemText(QCustomPlot* parentPlot);

	void setHTMLText(const QString& htmlText);

protected:
	virtual void draw(QCPPainter* painter) override;

private:
	void updateText()
	{
		// This function can be used to update any internal state if needed
	}

	QString mHTMLText;

};

