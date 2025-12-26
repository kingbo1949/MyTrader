#include "BracketWithArrow.h"

CBracketWithArrow::CBracketWithArrow(QCustomPlot* parentPlot)
	:QCPItemBracket(parentPlot)
{
	Init();
	this->setLayer("overlay");
}

void CBracketWithArrow::setArrow(bool useLeftArrow, bool useRightArrow)
{
	if (useLeftArrow)
	{
		m_twoArrow.arrow_left->setVisible(true);
	}
	if (useRightArrow)
	{
		m_twoArrow.arrow_right->setVisible(true);
	}
}

TwoArrow CBracketWithArrow::GetTwoArrow()
{
	return m_twoArrow;
}



void CBracketWithArrow::Init()
{

	m_twoArrow.arrow_left = new QCPItemLine(mParentPlot);
	m_twoArrow.arrow_left->setHead(QCPLineEnding::esSpikeArrow); // 设置箭头类型
	m_twoArrow.arrow_left->setLayer("overlay");
	m_twoArrow.arrow_left->setVisible(false);


	m_twoArrow.arrow_right = new QCPItemLine(mParentPlot);
	m_twoArrow.arrow_right->setHead(QCPLineEnding::esSpikeArrow); // 设置箭头类型
	m_twoArrow.arrow_right->setLayer("overlay");
	m_twoArrow.arrow_right->setVisible(false);

}

void CBracketWithArrow::draw(QCPPainter* painter)
{
	QCPVector2D leftVec(left->pixelPosition());
	QCPVector2D rightVec(right->pixelPosition());
	if (leftVec.toPoint() == rightVec.toPoint())
		return;

	QCPVector2D widthVec = (rightVec - leftVec) * 0.5;
	QCPVector2D lengthVec = widthVec.perpendicular().normalized() * mLength;
	QCPVector2D centerVec = (rightVec + leftVec) * 0.5 - lengthVec;

	QPolygon boundingPoly;
	boundingPoly << leftVec.toPoint() << rightVec.toPoint()
		<< (rightVec - lengthVec).toPoint() << (leftVec - lengthVec).toPoint();
	const int clipEnlarge = qCeil(mainPen().widthF());
	QRect clip = clipRect().adjusted(-clipEnlarge, -clipEnlarge, clipEnlarge, clipEnlarge);
	if (clip.intersects(boundingPoly.boundingRect()))
	{

		painter->setPen(mainPen());

		// 中横杠
		painter->drawLine((centerVec + widthVec).toPointF(), (centerVec - widthVec).toPointF());

		// right
		painter->drawLine((centerVec + widthVec).toPointF(), (centerVec + widthVec + lengthVec).toPointF());
		drawArrow(m_twoArrow.arrow_right, (centerVec + widthVec).toPointF(), (centerVec + widthVec + lengthVec).toPointF());

		// left
		painter->drawLine((centerVec - widthVec).toPointF(), (centerVec - widthVec + lengthVec).toPointF());
		drawArrow(m_twoArrow.arrow_left, (centerVec - widthVec).toPointF(), (centerVec - widthVec + lengthVec).toPointF());

	}
}


void CBracketWithArrow::drawArrow(QCPItemLine* arrow, QPointF devicePoint_begin, QPointF devicePoint_end)
{
	double customPlotX = mParentPlot->xAxis->pixelToCoord(devicePoint_begin.x());
	double customPlotY;
	if (mParentPlot->yAxis->visible())
	{
		customPlotY = mParentPlot->yAxis->pixelToCoord(devicePoint_begin.y());
	}
	else
	{
		customPlotY = mParentPlot->yAxis2->pixelToCoord(devicePoint_begin.y());
	}
	arrow->start->setCoords(customPlotX, customPlotY);

	customPlotX = mParentPlot->xAxis->pixelToCoord(devicePoint_end.x());
	if (mParentPlot->yAxis->visible())
	{
		customPlotY = mParentPlot->yAxis->pixelToCoord(devicePoint_end.y());
	}
	else
	{
		customPlotY = mParentPlot->yAxis2->pixelToCoord(devicePoint_end.y());
	}

	arrow->end->setCoords(customPlotX, customPlotY);
	arrow->setPen(mainPen());
	return;

}

