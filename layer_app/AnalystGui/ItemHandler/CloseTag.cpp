#include "CloseTag.h"

CCloseTag::CCloseTag(QCustomPlot* parent)
	:CItemsHandler(parent)
{
}

CCloseTag::~CCloseTag()
{
	;
}

void CCloseTag::SetKLines(const KlinePlotSuit& klinePlotSuit)
{
	DrawTag(klinePlotSuit);
}

void CCloseTag::UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount)
{
	DrawTag(klinePlotSuit);
}


AxisTagQPtr CCloseTag::MakeAndGet_AxisTag()
{
	if (!m_axisTag)
	{
		m_axisTag = new AxisTag(GetY());
		m_axisTag->setPen(QPen(QColor(250, 120, 0)));
	}
	return m_axisTag;
}

