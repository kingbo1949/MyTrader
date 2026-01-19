#pragma once
#include <QtWidgets>
#include <base_struc.h>

// 组合数据
struct KLine4Table
{
    IBKLinePtr		pkline;
    IBMacdPtr		pmacd;
    IBDivTypePtr	pDivType;
    IBAtrPtr		pAtr;
};
typedef std::vector<KLine4Table> KLine4Tables;

