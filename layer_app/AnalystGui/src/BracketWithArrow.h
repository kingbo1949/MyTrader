#pragma once

// 实现带箭头的中括号
#include "qcustomplot.h"

struct TwoArrow
{
	QCPItemLine* arrow_left;
	QCPItemLine* arrow_right;

};

class CBracketWithArrow : public QCPItemBracket
{
	Q_OBJECT
public:
	CBracketWithArrow(QCustomPlot* parentPlot);

	void		setArrow(bool useLeftArrow, bool useRightArrow);

	TwoArrow	GetTwoArrow();


protected:
	TwoArrow	m_twoArrow;
	void		Init();

	// 重写 draw 方法
	void		draw(QCPPainter* painter) override;

	void		drawArrow(QCPItemLine* arrow, QPointF devicePoint_begin, QPointF devicePoint_end);
};
typedef CBracketWithArrow* BracketWithArrowQPtr;
