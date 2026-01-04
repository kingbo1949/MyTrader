#include "Factory_AnalystGui.h"
#include <Global.h>
#include <QtGlobal.h>
#include <HighFrequencyGlobalFunc.h>
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

SubType g_subType = SubType::Macd;
void Set_SubType(SubType subMain)
{
	g_subType = subMain;
}

SubType Get_SubType()
{
	return g_subType;
}
SubVisible g_subVisible = SubVisible::Show;
void Set_SubVisible(SubVisible subVisible)
{
	g_subVisible = subVisible;
}

SubVisible Get_SubVisible()
{
	return g_subVisible;
}

