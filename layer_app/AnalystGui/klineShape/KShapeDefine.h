#pragma once

#include <memory>
#include <base_struc.h>
#include <QList>
#include <qstandarditemmodel.h>
enum class TopOrBottom
{
	Top,
	Bottom
};
std::string Trans_Str(TopOrBottom topOrBottom);

enum class KShape
{
	Normal,						// 正常
	Div,						// 背离
	DblDiv,						// 双重背离（距离远）
	DblDivPlus,					// 加强版双重背离（距离近）
	TrainDiv					// 轨道式背离
};
std::string Trans_Str(KShape kShape);


struct KShapeOutput
{
	CodeStr				codeId;
	Time_Type			timeType;
	TopOrBottom			topOrBottom;
	KShape				kShape;
	int					signalPosition;		// 信号位置（0表示当前BAR，-1表示上一个BAR,依此类推）
	
};
typedef std::vector<KShapeOutput> KShapeOutputs;

typedef QList<QStandardItem*> TableViewRecord;		// 一行记录

// 一天内的K线数量
int BarsInDay(Time_Type timetype, SecurityType securityType);

// 检查K线形态时需要回溯的K线数量
int CheckBackKLineCount(Time_Type timetype, SecurityType securityType);

// 判断两者是否兼容
bool IsCompatible(TopOrBottom topOrBottom, DivergenceType divergenceType);

// 判断是否遇见指定的信号
bool IsMeetSignal(TopOrBottom topOrBottom, IBDivTypePtr signal);
