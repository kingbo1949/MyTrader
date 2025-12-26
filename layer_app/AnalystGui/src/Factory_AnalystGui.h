#pragma once

#include "qcustomplot.h"
#include <base_struc.h>
#include <AnalystDealDefine.h>
#include "../src/ColorManager.h"
#include "../tSideTimeHandler/TSideTimeHandler.h"
#include <Factory_Calculator.h>

enum class MainOrSub
{
	MainT,
	SubT
};


typedef std::vector<QPointF> QPointFs;

typedef QCPItemLine* QCPItemLineQPtr;
typedef std::vector<QCPItemLineQPtr> QCPItemLineQPtrs;

typedef QCPAbstractItem* QCPAbstractItemQPtr;
typedef std::vector<QCPAbstractItemQPtr> QCPAbstractItemQPtrs;

typedef QCPMarginGroup* QCPMarginGroupQPtr;
typedef std::vector<QCPAbstractItemQPtr> QCPAbstractItemQPtrs;

typedef QCPFinancial* QCPFinancialQPtr;
typedef std::vector<QCPFinancialQPtr> QCPFinancialQPtrs;

typedef QCPItemRect* QCPItemRectQPtr;
typedef std::vector<QCPItemRectQPtr> QCPItemRectQPtrs;

typedef QCPCurve* QCPCurveQPtr;
typedef std::vector<QCPCurveQPtr> QCPCurveQPtrs;

typedef QCPGraph* QCPGraphQPtr;
typedef std::vector<QCPGraphQPtr> QCPGraphQPtrs;

typedef QCPBars* QCPBarsQPtr;
typedef std::vector<QCPBarsQPtr> QCPBarsQPtrs;

typedef QCPAxisRect* QCPAxisRectQPtr;
typedef std::vector<QCPAxisRectQPtr> QCPAxisRectQPtrs;

typedef QCPAxis* QCPAxisQPtr;
typedef std::vector<QCPAxisQPtr> QCPAxisQPtrs;

typedef QSharedPointer<QCPAxisTickerText> XTickerQSPtr;



typedef int KLineIndex;
typedef std::map<Tick_T, KLineIndex> Time_Index;

typedef QCPDataContainer<QCPFinancialData> YContainer;


struct KlineChgCount
{
	int addcount = 0;
	int chgcount = 0;
};


struct KlinePlotSuit
{
	CodeStr				codeId;
	Time_Type			timeType;

	Time_Index			time_index;		// k线时间与klines中index的map, 通过它来判断新来的BAR是更新还是添加
	IBKLinePtrs			klines;

	SimpleMatchPtrs		tMatches;
};

enum class TableViewMouse
{
	Mouse = 0,
	NoMouse = 1
};



// 得到Y轴的伸缩因子
double					GetYAxisFactor();
double					GetXAxisFactor();


// 颜色
ColorManagerPtr			MakeAndGet_ColorManager();

TSideTimeHandlerPtr		MakeAndGet_TSideTimeHandler(SecurityType securityType);

// k线宽度
void					SetKlineWidth(double width);
double					GetKlineWidth();

// 得到主附图轴区域指针
QCPAxisRectQPtr			GetAxisRect(MainOrSub mainOrSub);
void					SetAxisRect(MainOrSub mainOrSub, QCPAxisRectQPtr axisRect);

// 按主图x轴范围，设置y轴
void					SetMainY(const QCPRange& xRange, const IBKLinePtrs& klinesInSuit);

Container_MacdPtr		MakeAndGet_MacdContainer();
Container_AveragePtr	MakeAndGet_EmaContainer();
Container_AveragePtr	MakeAndGet_MaContainer();
Container_MacdDivPtr	MakeAndGet_MacdDivContainer(Container_MacdPtr macdContainer);

void					Set_TableViewMouse(TableViewMouse tableViewMouse);
TableViewMouse			Get_TableViewMouse();






template <typename T>
void DelData(int beginPos, T* obj)
{
	obj->data()->removeAfter(beginPos - 0.000001);
}


