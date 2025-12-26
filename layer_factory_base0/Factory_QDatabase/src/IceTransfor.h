#pragma once
#include "QDatabaseDefine.h"
#include <base_struc.h>
#include <QStruc.h>
using namespace IBTrader;

class CIceTransfor
{
public:
	CIceTransfor() { ; };
	virtual ~CIceTransfor() { ; };


	static IBTickPtr					TransTickIceToMy(const ITick& tick);

	static ITick						TransTickMyToIce(IBTickPtr tick);

	static IQuery						TransQueryMyToIce(const QQuery& query);



	static IBKLinePtr					TransKLineIceToMy(const IKLine& kline);

	static IKLine						TransKLineMyToIce(const IBKLinePtr& kline);

	static ITimeType					TransTimeTypeToIce(Time_Type timeType);

	// 转换MA
	static IBAvgValuePtr				TransAvgValueIceToMy(const IAvgValue& value);
	static IAvgValue					TransAvgValueMyToIce(const IBAvgValuePtr& ma);

	// 转换klinepair
	static IBKLinePair					TransKLinePairIceToMy(const IKLinePair& klinePair);

	static IKLinePair					TransKLinePairMyToIce(const IBKLinePair& klinePair);

	// 转换macd
	static IBMacdPtr					TransMacd(const IMacdValue& value);
	static IMacdValue					TransMacd(const IBMacdPtr pValue);

	// 转换DivType
	static IBDivTypePtr					TransDivType(const IDivTypeValue& value);
	static IDivTypeValue				TransDivType(IBDivTypePtr pValue);

};

