#include "HtmlItemText.h"

CHtmlItemText::CHtmlItemText(QCustomPlot* parentPlot)
	:QCPItemText(parentPlot)
{

}

void CHtmlItemText::setHTMLText(const QString& htmlText)
{
	mHTMLText = htmlText;
	updateText();
}

void CHtmlItemText::draw(QCPPainter* painter)
{
	if (mHTMLText.isEmpty())
		return;

	painter->setFont(mainFont());
	QPointF pos = position->pixelPosition();

	QTextDocument doc;
	doc.setHtml(mHTMLText);
	doc.setDefaultFont(mainFont());

	painter->save();
	painter->translate(pos);
	doc.drawContents(painter);
	painter->restore();
}

