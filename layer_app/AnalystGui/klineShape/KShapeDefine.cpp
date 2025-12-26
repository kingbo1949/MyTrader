#include "KShapeDefine.h"

std::string Trans_Str(TopOrBottom topOrBottom)
{
	if (topOrBottom == TopOrBottom::Top)
	{
		return "Top";
	}
	else
	{
		return "Btm";
	}
}

std::string Trans_Str(KShape kShape)
{
	switch (kShape)
	{
	case KShape::Normal:
		return "Normal";
	case KShape::Div:
		return "Div";
	case KShape::DblDiv:
		return "DblDiv";
	case KShape::DblDivPlus:
		return "DblDiv+";
	case KShape::TrainDiv:
		return "TrainDiv";
	default:
		return "Unknown";
	}
}

int BarsInDay(Time_Type timetype, SecurityType securityType)
{
	int ret = 0;

	if (securityType == SecurityType::FUT)
	{
		switch (timetype)
		{
		case Time_Type::S15:
			break;
		case Time_Type::M1:
			ret = 1380;
			break;
		case Time_Type::M5:
			ret = 276;
			break;
		case Time_Type::M15:
			ret = 91;
			break;
		case Time_Type::M30:
			ret = 46;
			break;
		case Time_Type::H1:
			ret = 23;
			break;
		case Time_Type::D1:
			break;
		default:
			break;
		}
		return ret;
	}
	else
	{
		switch (timetype)
		{
		case Time_Type::S15:
			break;
		case Time_Type::M1:
			ret = 960;
			break;
		case Time_Type::M5:
			ret = 192;
			break;
		case Time_Type::M15:
			ret = 64;
			break;
		case Time_Type::M30:
			ret = 32;
			break;
		case Time_Type::H1:
			ret = 16;
			break;
		case Time_Type::D1:
			break;
		default:
			break;
		}
		return ret;
	}


	
}

int CheckBackKLineCount(Time_Type timetype, SecurityType securityType)
{
	// 为了检查K线形态，通常需要回溯一定数量的K线，选择回溯2天
	int days = 2;

	// 日线特殊处理，直接返回10根
	if (timetype == Time_Type::D1) return 10;

	int barsInDay = BarsInDay(timetype, securityType);
	return barsInDay * days;
}

bool IsCompatible(TopOrBottom topOrBottom, DivergenceType divergenceType)
{
	if (divergenceType == DivergenceType::Normal) return true;

	if (divergenceType == DivergenceType::Bottom || divergenceType == DivergenceType::BottomSub)
	{
		if (topOrBottom == TopOrBottom::Bottom) return true;
		return false;
	}
	else if (divergenceType == DivergenceType::Top || divergenceType == DivergenceType::TopSub)
	{
		if (topOrBottom == TopOrBottom::Top) return true;
		return false;
	}
	return true;;
}

bool IsMeetSignal(TopOrBottom topOrBottom, IBDivTypePtr signal)
{
	if (signal->divType == DivergenceType::Normal) return false;
	if (!signal->isUTurn) return false;

	if (topOrBottom == TopOrBottom::Top)
	{
		if (signal->divType == DivergenceType::Top || signal->divType == DivergenceType::TopSub)
		{
			return true;
		}
	}
	else if (topOrBottom == TopOrBottom::Bottom)
	{
		if (signal->divType == DivergenceType::Bottom || signal->divType == DivergenceType::BottomSub)
		{
			return true;
		}
	}
	return false;
}
