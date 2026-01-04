#include "PlotContainer.h"
#include <Factory_IBGlobalShare.h>
#include <Global.h>
#include <QtGlobal.h>
#include "AxisTickerText_KLine.h"
#include "Factory_AnalystGui.h"
#include "../ItemHandler/TMatchesItems.h"
#include "../ItemHandler/MouseMoveItems.h"
#include "../ItemHandler/TradeSideRect.h"
#include "../ItemHandler/KLinePlot.h"
#include "../ItemHandler/MaGraph.h"
#include "../ItemHandler/VwMaGraph.h"
#include "../ItemHandler/MacdGraph.h"
#include "../ItemHandler/CloseTag_Main.h"
#include "../ItemHandler/CloseTag_Sub.h"
#include "../ItemHandler/EmaGraph.h"
#include "../ItemHandler/MacdDivergenceItem.h"
#include "../ItemHandler/LineDrawer.h"
#include "../ItemHandler/AtrGraph.h"
CPlotContainer::CPlotContainer(QCustomPlot* parent)
	:CPlotObject(parent)
{
	m_textTicker = QSharedPointer<CAxisTickerText_KLine>::create();
	m_marginGroup = new QCPMarginGroup(m_customPlot);

	GetAxisRect(MainOrSub::MainT)->setAutoMargins(QCP::MarginSide::msLeft | QCP::MarginSide::msRight | QCP::MarginSide::msBottom);
	GetAxisRect(MainOrSub::MainT)->setMarginGroup(QCP::msLeft | QCP::MarginSide::msRight, m_marginGroup);
	GetAxisRect(MainOrSub::SubT)->setAutoMargins(QCP::MarginSide::msLeft | QCP::MarginSide::msRight | QCP::MarginSide::msBottom);
	GetAxisRect(MainOrSub::SubT)->setMarginGroup(QCP::msLeft | QCP::MarginSide::msRight, m_marginGroup);

	GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atRight)->setPadding(35);

	InitItems();

	m_klineUpdator = std::make_shared<CKLineUpdator>();

	// 启用鼠标交互
	m_customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);


	// 连接鼠标移动事件
	connect(m_customPlot, &QCustomPlot::mouseMove, this, &CPlotContainer::onMouseMove);
	connect(m_customPlot, &QCustomPlot::mouseRelease, this, &CPlotContainer::onMouseRelease);
	connect(m_customPlot, &QCustomPlot::mouseWheel, this, &CPlotContainer::onMouseWheel);

	connect(GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atBottom), SIGNAL(rangeChanged(QCPRange)),
		GetAxisRect(MainOrSub::SubT)->axis(QCPAxis::atBottom), SLOT(setRange(QCPRange)));

	// 附图禁止拖动
	GetAxisRect(MainOrSub::SubT)->setRangeDrag(Qt::Orientation(0));

}
void CPlotContainer::InitItems()
{
	m_items.push_back(new CKLinePlot(m_customPlot, m_textTicker));
	m_items.push_back(new CTMatchesItems(m_customPlot));
	m_items.push_back(new CMouseMoveItems(m_customPlot));
	m_items.push_back(new CTradeSideRect(m_customPlot));
	m_items.push_back(new CCloseTag_Main(m_customPlot));
	m_items.push_back(new CMaGraph(m_customPlot));
	//m_items.push_back(new CVwMaGraph(m_customPlot));
	//m_items.push_back(new CEmaGraph(m_customPlot));
	m_items.push_back(new CAtrGraph(m_customPlot, SubType::Atr));
	m_items.push_back(new CMacdGraph(m_customPlot, SubType::Macd));
	m_items.push_back(new CMacdDivergenceItem(m_customPlot, SubType::Macd));
	m_items.push_back(new CCloseTag_Sub(m_customPlot)); 
	m_items.push_back(new CLineDrawer(m_customPlot));


}

void CPlotContainer::SetKlines(const CodeStr& codeId, Time_Type timeType, const IBKLinePtrs& klines)
{
	m_klinePlotSuit.time_index.clear();
	m_klinePlotSuit.klines.clear();

	if (codeId != m_klinePlotSuit.codeId)
	{
		m_klinePlotSuit.tMatches.clear();
		PlusSetMatches();
	}

	m_klinePlotSuit.codeId = codeId;
	m_klinePlotSuit.timeType = timeType;

	m_klineUpdator->Update(klines, m_klinePlotSuit);

	PlusSetKLines();


	m_customPlot->plotLayout()->update(QCPLayoutElement::UpdatePhase::upLayout);
	m_customPlot->replot();
}

void CPlotContainer::AddKlines(const CodeStr& codeId, Time_Type timeType, const IBKLinePtrs& klines)
{
	// ontime查了末尾的两根线来更新m_klinePlotSuit
	if (codeId != m_klinePlotSuit.codeId) return;
	if (timeType != m_klinePlotSuit.timeType) return;
	if (klines.empty()) return;
	if (m_klinePlotSuit.klines.empty()) return;

	// 开始用最近的2根K线与原来suit数据比较
	int addcount = m_klineUpdator->Update(klines, m_klinePlotSuit);
	int chgcount = int(klines.size()) - addcount;

	PlusUpdateKlines(addcount, chgcount);
	m_customPlot->replot();

}

void CPlotContainer::SetTMatches(SimpleMatchPtrs matches)
{
	m_klinePlotSuit.tMatches = matches;
	PlusSetMatches();
	m_customPlot->layer("overlay")->replot();
}


void CPlotContainer::onMouseMove(QMouseEvent* event)
{

	// 获取鼠标在 QCustomPlot 中的坐标
	if (m_klinePlotSuit.klines.empty()) return;
	for (auto item : m_items)
	{
		item->MouseMoveEvent(event, m_klinePlotSuit);
	}
	m_customPlot->layer("overlay")->replot();

}

void CPlotContainer::SubTypeChg()
{
	if (m_klinePlotSuit.klines.empty()) return;;
	for (auto item : m_items)
	{
		item->SubTypeChg(m_klinePlotSuit);
	}
	return;

}

void CPlotContainer::GoHome()
{
	int count = 300;
	QCPRange xRange = GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atBottom)->range();
	if (xRange.upper - xRange.lower < count)
	{
		count = xRange.upper - xRange.lower;
	}


	if (m_klinePlotSuit.klines.size() < count) return;
	xRange.lower = 0;
	xRange.upper = count;
	GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atBottom)->setRange(xRange);
	//SetMainY(xRange, m_klinePlotSuit.klines);

	onMouseWheel(nullptr);

}

void CPlotContainer::GoEnd()
{
	int count = 300;
	QCPRange xRange = GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atBottom)->range();
	if (xRange.upper - xRange.lower < count)
	{
		count = xRange.upper - xRange.lower;
	}


	if (m_klinePlotSuit.klines.size() < count) return;
	size_t begin = m_klinePlotSuit.klines.size() - count;
	size_t end = m_klinePlotSuit.klines.size() - 1;
	xRange.lower = begin;
	xRange.upper = end + 5;
	GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atBottom)->setRange(xRange);
	//SetMainY(xRange, m_klinePlotSuit.klines);

	onMouseWheel(nullptr);
}

void CPlotContainer::onMouseRelease(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		// 发生了鼠标拖拽操作
		// 使用QTimer::singleShot来延迟执行自定义行为
		QTimer::singleShot(5, this, &CPlotContainer::ChangeAxisRangeForMouse);
	}
}

void CPlotContainer::onMouseWheel(QWheelEvent* event)
{
	// 鼠标缩放事件
	// 使用QTimer::singleShot来延迟执行自定义行为
	QTimer::singleShot(5, this, &CPlotContainer::ChangeAxisRangeForMouse);
}


void CPlotContainer::ChangeAxisRangeForMouse()
{
	// 坐标变化事件
	for (auto item : m_items)
	{
		if (m_klinePlotSuit.klines.empty()) continue;
		item->AxisRangeChgEvent(m_klinePlotSuit);
	}
	m_customPlot->replot();

	return;
}

void CPlotContainer::PlusSetKLines()
{
	if (m_klinePlotSuit.klines.empty()) return;;
	for (auto item : m_items)
	{

		item->SetKLines(m_klinePlotSuit);
	}
	return;
}

void CPlotContainer::PlusUpdateKlines(int addcount, int chgcount)
{
	if (m_klinePlotSuit.klines.empty()) return;;

	KlineChgCount klineChgCount;
	klineChgCount.addcount = addcount;
	klineChgCount.chgcount = chgcount;
	for (auto item : m_items)
	{
		item->UpdateKlines(m_klinePlotSuit, klineChgCount);
	}
	return;
}

void CPlotContainer::PlusSetMatches()
{
	for (auto item : m_items)
	{
		item->SetMatches(m_klinePlotSuit);
	}
	return;
}


