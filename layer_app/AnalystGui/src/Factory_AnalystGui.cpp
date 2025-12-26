#include "Factory_AnalystGui.h"
#include <Global.h>
#include <QtGlobal.h>
#include <HighFrequencyGlobalFunc.h>
#include <Factory_Calculator.h>
#include "../tSideTimeHandler/TSideTimeHandler_Future.h"
#include "../tSideTimeHandler/TSideTimeHandler_Stock.h"


const QColor BackGround;
const QColor BrushPositive;
const QColor PenPositive;
const QColor BrushNegative;
const QColor PenNegative;

const QColor Line5;
const QColor Line20;
const QColor Line60;
const QColor Line200;

const QColor LineProfit;
const QColor LineLoss;
const QColor ArrowBuy;
const QColor ArrowSell;


double GetYAxisFactor()
{
	return 1.08;
}

double GetXAxisFactor()
{
	return 1.08;
}

ColorManagerPtr g_pColorManager = nullptr;
ColorManagerPtr MakeAndGet_ColorManager()
{
	if (!g_pColorManager)
	{
		g_pColorManager = std::make_shared<CColorManager>();
	}
	return g_pColorManager;
}

TSideTimeHandlerPtr g_handler_stock = nullptr;
TSideTimeHandlerPtr g_handler_future = nullptr;

TSideTimeHandlerPtr MakeAndGet_TSideTimeHandler(SecurityType securityType)
{
	if (securityType == SecurityType::FUT)
	{
		if (!g_handler_future)
		{
			g_handler_future = std::make_shared<CTSideTimeHandler_Future>();
		}
		return g_handler_future;
	}
	else
	{
		if (!g_handler_stock)
		{
			g_handler_stock = std::make_shared<CTSideTimeHandler_Stock>();
		}
		return g_handler_stock;
	}

	return nullptr;
}

double g_klinewidth = 0;
void SetKlineWidth(double width)
{
	g_klinewidth = width;
}

double GetKlineWidth()
{
	return g_klinewidth;
}

QCPAxisRectQPtr g_mainRect = nullptr;
QCPAxisRectQPtr g_subRect = nullptr;

QCPAxisRectQPtr GetAxisRect(MainOrSub mainOrSub)
{
	if (mainOrSub == MainOrSub::MainT)
	{
		return g_mainRect;
	}
	else
	{
		return g_subRect;
	}
}
void SetAxisRect(MainOrSub mainOrSub, QCPAxisRectQPtr axisRect)
{
	if (mainOrSub == MainOrSub::MainT)
	{
		g_mainRect = axisRect;
	}
	else
	{
		g_subRect = axisRect;
	}
}

void SetMainY(const QCPRange& xRange, const IBKLinePtrs& klinesInSuit)
{
	int beginPos = std::floor(xRange.lower);
	if (beginPos < 0)
	{
		beginPos = 0;
	}

	int endPos = std::ceil(xRange.upper);
	if (endPos >= int(klinesInSuit.size()))
	{
		endPos = int(klinesInSuit.size());
	}
	// 截取视窗内k线
	IBKLinePtrs klines = IBKLinePtrs(klinesInSuit.begin() + beginPos, klinesInSuit.begin() + endPos);
	HighAndLow highLow = CHighFrequencyGlobalFunc::MakeHighAndLow(klines, false);

	QCPRange range(highLow.low, highLow.high);
	GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atRight)->setRange(range);
	GetAxisRect(MainOrSub::MainT)->axis(QCPAxis::atRight)->scaleRange(GetYAxisFactor(), range.center());


}
Container_MacdPtr g_pMacdContainer = nullptr;
Container_MacdPtr MakeAndGet_MacdContainer()
{
	if (!g_pMacdContainer)
	{
		g_pMacdContainer = Make_Container_Macd();
	}
	return g_pMacdContainer;
}
Container_AveragePtr g_pEmaContainer = nullptr;
Container_AveragePtr MakeAndGet_EmaContainer()
{
	if (!g_pEmaContainer)
	{
		g_pEmaContainer = Make_Container_Ema();
	}
	return g_pEmaContainer;
}

Container_AveragePtr g_pMaContainer = nullptr;
Container_AveragePtr MakeAndGet_MaContainer()
{
	if (!g_pMaContainer)
	{
		g_pMaContainer = Make_Container_Ma();
	}
	return g_pMaContainer;
}
Container_MacdDivPtr g_pMacdDivContainer = nullptr;
Container_MacdDivPtr MakeAndGet_MacdDivContainer(Container_MacdPtr macdContainer)
{
	if (!g_pMacdDivContainer)
	{
		g_pMacdDivContainer = Make_Container_MacdDiv(macdContainer);
	}
	return g_pMacdDivContainer;
}

TableViewMouse g_tableViewMouse = TableViewMouse::NoMouse;
void Set_TableViewMouse(TableViewMouse tableViewMouse)
{
	g_tableViewMouse = tableViewMouse;
}

TableViewMouse Get_TableViewMouse()
{
	return g_tableViewMouse;
}



