#include "MouseMoveItems.h"
#include "../src/Factory_AnalystGui.h"
#include <Global.h>
#include <QtGlobal.h>
#include <Factory_QDatabase.h>
CMouseMoveItems::CMouseMoveItems(QCustomPlot* parent)
	:CItemsHandler(parent)
{
	Init();
}

void CMouseMoveItems::MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit)
{
	DrawToolTip(klinePlotSuit.codeId, klinePlotSuit.timeType, event, klinePlotSuit.klines);
	DrawCrossLine(event, klinePlotSuit.klines);

}



QCPAxisQPtr CMouseMoveItems::GetX()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atBottom);

}

QCPAxisQPtr CMouseMoveItems::GetY()
{
	return GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atRight);
}

QCPAxisQPtr CMouseMoveItems::GetSubX()
{
	return GetAxisRect(MainOrSub::SubT)->axis(QCPAxis::atBottom);
}

QCPAxisQPtr CMouseMoveItems::GetSubY()
{

	return GetAxisRect(MainOrSub::SubT)->axis(QCPAxis::atRight);
}

void CMouseMoveItems::Init()
{
	m_crossLineItems = std::make_shared<CrossLineItems>();

	m_crossLineItems->vLine = new QCPItemLine(m_customPlot);
	m_crossLineItems->vLine->setPen(QPen(Qt::DashLine));
	m_crossLineItems->vLine->setLayer("overlay");
	m_crossLineItems->vLine->start->setCoords(GetX()->range().lower, GetY()->range().lower);
	m_crossLineItems->vLine->end->setCoords(GetX()->range().lower, GetY()->range().lower);
	m_crossLineItems->vLine->start->setAxes(GetX(), GetY());
	m_crossLineItems->vLine->end->setAxes(GetX(), GetY());
	m_crossLineItems->vLine->setClipAxisRect(nullptr);
	//m_crossLineItems->vLine->setIn setInterpolating


	m_crossLineItems->hLine = new QCPItemLine(m_customPlot);
	m_crossLineItems->hLine->setPen(QPen(Qt::DashLine));
	m_crossLineItems->hLine->setLayer("overlay");
	m_crossLineItems->hLine->start->setCoords(GetX()->range().lower, GetY()->range().lower);
	m_crossLineItems->hLine->end->setCoords(GetX()->range().lower, GetY()->range().lower);
	m_crossLineItems->hLine->start->setAxes(GetX(), GetY());
	m_crossLineItems->hLine->end->setAxes(GetX(), GetY());
	m_crossLineItems->hLine->setClipAxisRect(nullptr);


	m_crossLineItems->hTextItem = new QCPItemText(m_customPlot);
	m_crossLineItems->hTextItem->setPositionAlignment(Qt::AlignBottom | Qt::AlignHCenter);
	m_crossLineItems->hTextItem->position->setType(QCPItemPosition::ptPlotCoords);
	m_crossLineItems->hTextItem->setText("");
	m_crossLineItems->hTextItem->setFont(QFont(m_customPlot->font().family(), 10));
	//m_crossLineItems->hTextItem->setPen(QPen(Qt::black));
	m_crossLineItems->hTextItem->setColor(Qt::white); // 设置文本颜色为白色
	m_crossLineItems->hTextItem->setBrush(QBrush(Qt::black)); // 设置背景颜色为黑色
	m_crossLineItems->hTextItem->setLayer("overlay");
	m_crossLineItems->hTextItem->position->setAxes(GetX(), GetY());

	m_crossLineItems->vTextItem = new QCPItemText(m_customPlot);
	m_crossLineItems->vTextItem->setPositionAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_crossLineItems->vTextItem->position->setType(QCPItemPosition::ptPlotCoords);
	m_crossLineItems->vTextItem->setText("");
	m_crossLineItems->vTextItem->setFont(QFont(m_customPlot->font().family(), 10));
	//m_crossLineItems->vTextItem->setPen(QPen(Qt::black));
	m_crossLineItems->vTextItem->setColor(Qt::white); // 设置文本颜色为白色
	m_crossLineItems->vTextItem->setBrush(QBrush(Qt::black)); // 设置背景颜色为黑色
	m_crossLineItems->vTextItem->setLayer("overlay");
	m_crossLineItems->vTextItem->position->setAxes(GetX(), GetY());

	return;

}

void CMouseMoveItems::DrawToolTip(const CodeStr& codeId, Time_Type timeType, QMouseEvent* event, const IBKLinePtrs& klines)
{
	// 获取鼠标在 QCustomPlot 中的坐标
	if (klines.empty()) return;


	double x = GetX()->pixelToCoord(event->pos().x());
	double y = GetY()->pixelToCoord(event->pos().y());
	//qDebug() << "x: " << x << "y: " << y;

	// 查找最近的数据点
	int dictX = qRound(x);
	if (dictX < 0)
	{
		dictX = 0;
	}
	if (dictX >= klines.size())
	{
		dictX = int(klines.size()) - 1;
	}

	IBKLinePtr kline = klines[dictX];

	IBMacdPtr pMacd = MakeAndGet_QDatabase()->GetOneMacd(codeId, timeType, kline->time);
	if (!pMacd) return;

	double avgatr = -1;
	IBAtrPtr pAtr = MakeAndGet_QDatabase()->GetOneAtr(codeId, timeType, kline->time);
	if (pAtr)
	{
		avgatr = pAtr->avgAtr;
	}

	QString info = QString("Index: %1\nOpen: %2\nClose: %3\nLow: %4\nHigh: %5\nMacd_Diff: %6\nMacd_Dea: %7\nMacd_Bar: %8\nAtr: %9")
		.arg(dictX)
		.arg(QString::number(kline->open, 'f', 2))
		.arg(QString::number(kline->close, 'f', 2))
		.arg(QString::number(kline->low, 'f', 2))
		.arg(QString::number(kline->high, 'f', 2))
		.arg(QString::number(pMacd->dif, 'f', 2))
		.arg(QString::number(pMacd->dea, 'f', 2))
		.arg(QString::number(pMacd->macd, 'f', 2))
		// .arg(QString::number(pAtr->avgAtr, 'f', 2))
		.arg(QString::number(avgatr, 'f', 2))
		;
	QToolTip::showText(event->globalPosition().toPoint(), info, m_customPlot);

}

void CMouseMoveItems::DrawCrossLine(QMouseEvent* event, const IBKLinePtrs& klines)
{
	double x = GetX()->pixelToCoord(event->pos().x());
	double y = GetY()->pixelToCoord(event->pos().y());
	double y1 = GetSubY()->pixelToCoord(event->pos().y());
	//qDebug() << "y:" << y;
	//qDebug() << "y1:" << y1;

	// 查找最近的数据点
	int dictX = qRound(x);
	if (dictX < 0)
	{
		dictX = 0;
	}
	if (dictX >= klines.size())
	{
		dictX = int(klines.size()) - 1;
	}

	IBKLinePtr kline = klines[dictX];


	// 绘制十字虚线
	if (m_customPlot->plotLayout()->elementCount() == 1)
	{
		// 附图被删除
		m_crossLineItems->vLine->start->setAxes(GetX(), GetY());
		m_crossLineItems->vLine->start->setCoords(x, GetY()->range().lower);
	}
	else
	{
		// 主图附图完备
		m_crossLineItems->vLine->start->setAxes(GetSubX(), GetSubY());
		m_crossLineItems->vLine->start->setCoords(x, GetSubY()->range().lower);
	}
	m_crossLineItems->vLine->end->setCoords(x, GetY()->range().upper);

	m_crossLineItems->hLine->start->setCoords(GetX()->range().lower, y);
	m_crossLineItems->hLine->end->setCoords(GetX()->range().upper, y);


	// 绘制十字虚线注释
	std::string timestr = CGlobal::GetTickTimeStr(kline->time);
	QString time = CQtGlobal::StdString_QString(timestr);

	m_crossLineItems->hTextItem->setText(time);
	m_crossLineItems->hTextItem->position->setCoords(x, GetY()->range().lower); // 设置水平虚线与纵轴相交点的位置

	QPoint mousePos = event->pos(); // 获取鼠标当前位置（像素坐标）
	QCPAxisRect* hoveredRect = m_customPlot->axisRectAt(mousePos); // 获取鼠标所在的坐标区域

	if (hoveredRect == GetAxisRect(MainOrSub::MainT))
	{
		//qDebug() << "鼠标在主图区域";
		m_crossLineItems->vTextItem->position->setAxes(GetX(), GetY());
		m_crossLineItems->vTextItem->setClipAxisRect(GetAxisRect(MainOrSub::MainT));
		m_crossLineItems->vTextItem->setText(QString::number(y));
		m_crossLineItems->vTextItem->position->setCoords(GetX()->range().upper, y); // 设置垂直虚线与横轴相交点的位置

	}
	else if (hoveredRect == GetAxisRect(MainOrSub::SubT))
	{
		//qDebug() << "鼠标在附图区域";
		m_crossLineItems->vTextItem->position->setAxes(GetSubX(), GetSubY());
		m_crossLineItems->vTextItem->setClipAxisRect(GetAxisRect(MainOrSub::SubT));
		m_crossLineItems->vTextItem->setText(QString::number(y1));
		m_crossLineItems->vTextItem->position->setCoords(GetSubX()->range().upper, y1); // 设置垂直虚线与横轴相交点的位置

	}
	else
	{
		//qDebug() << "鼠标不在任何已知坐标区域";
		;
	}


	return;

}
