#include "pch.h"
#include "TransToStr.h"
#include <Factory_Log.h>
CTransToStr::CTransToStr()
{

}

CTransToStr::~CTransToStr()
{

}


std::string CTransToStr::Get_Bool(bool trueOrFalse)
{
	if (trueOrFalse) return "True";
	return "False";
}

bool CTransToStr::Get_Bool(const std::string& trueOrFalse)
{
	if (TransToLowerCaseOrUpperCase(trueOrFalse, UpperOrLower::LowerCase) == "true")
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::string CTransToStr::Get_SecurityType(SecurityType securityType)
{
	if (securityType == SecurityType::STK) return "STK";
	if (securityType == SecurityType::FUT) return "FUT";

	Log_Print(LogLevel::Err, fmt::format("unknows SecurityType Type:{}", int(securityType)));
	exit(-1);
	return "";
}

SecurityType CTransToStr::Get_SecurityType(const std::string& SecurityTypeStr)
{
	if (SecurityTypeStr == "STK") return SecurityType::STK;
	if (SecurityTypeStr == "FUT") return SecurityType::FUT;

	Log_Print(LogLevel::Err, fmt::format("unknows SecurityType :{}", SecurityTypeStr.c_str()));
	exit(-1);
	return SecurityType::STK;
}


std::string CTransToStr::Get_ExchangePl(ExchangePl exchangePl)
{
	if (exchangePl == ExchangePl::SMART) return "SMART";
	if (exchangePl == ExchangePl::SEHK) return "SEHK";
	if (exchangePl == ExchangePl::NYSE) return "NYSE";
	if (exchangePl == ExchangePl::NASDAQ) return "NASDAQ";
	if (exchangePl == ExchangePl::ARCA) return "ARCA";
	if (exchangePl == ExchangePl::CME) return "CME";
	if (exchangePl == ExchangePl::COMEX) return "COMEX";

	Log_Print(LogLevel::Err, fmt::format("unknows ExchangePl Type:{}", int(exchangePl)));
	exit(-1);
	return "";
}

ExchangePl CTransToStr::Get_ExchangePl(const std::string& exchangePlStr)
{
	if (exchangePlStr == "SMART") return ExchangePl::SMART;
	if (exchangePlStr == "SEHK") return ExchangePl::SEHK;
	if (exchangePlStr == "NYSE") return ExchangePl::NYSE;
	if (exchangePlStr == "NASDAQ") return ExchangePl::NASDAQ;
	if (exchangePlStr == "ARCA") return ExchangePl::ARCA;
	if (exchangePlStr == "CME") return ExchangePl::CME;
	if (exchangePlStr == "COMEX") return ExchangePl::COMEX;

	Log_Print(LogLevel::Err, fmt::format("unknows ExchangePl Type:{}", exchangePlStr.c_str()));
	exit(-1);
	return ExchangePl::SMART;
}

//enum class Money
//{
//	RMB = 0,						//人民币
//	USD = 1,						//美元
//	HKD = 2,						//港币
//	GBP = 3,						//英镑
//	JPY = 4,						//日元
//	CAD = 5,						//加拿大元
//	AUD = 6,						//澳大利亚元
//	EUR = 7,						//殴元
//	KRW = 8,						//韩元
//};

std::string CTransToStr::Get_Money(Money money)
{
	if (money == Money::RMB) return "RMB";
	if (money == Money::USD) return "USD";
	if (money == Money::HKD) return "HKD";
	if (money == Money::GBP) return "GBP";
	if (money == Money::JPY) return "JPY";
	if (money == Money::CAD) return "CAD";
	if (money == Money::AUD) return "AUD";
	if (money == Money::EUR) return "EUR";
	if (money == Money::KRW) return "KRW";

	Log_Print(LogLevel::Err, fmt::format("unknows Money Type:{}", int(money)));
	exit(-1);
	return "";
}

Money CTransToStr::Get_Money(const std::string& moneyStr)
{
	if (moneyStr == "RMB") return Money::RMB;
	if (moneyStr == "USD") return Money::USD;
	if (moneyStr == "HKD") return Money::HKD;
	if (moneyStr == "GBP") return Money::GBP;
	if (moneyStr == "JPY") return Money::JPY;
	if (moneyStr == "CAD") return Money::CAD;
	if (moneyStr == "AUD") return Money::AUD;
	if (moneyStr == "EUR") return Money::EUR;
	if (moneyStr == "KRW") return Money::KRW;

	Log_Print(LogLevel::Err, fmt::format("unknows Money Type:{}", moneyStr.c_str()));
	exit(-1);
	return Money::USD;
}

std::string CTransToStr::Get_SetupType(SetupType setupType)
{
	if (setupType == SetupType::QGenerator) return "QGenerator";
	if (setupType == SetupType::RealTrader) return "RealTrader";
	if (setupType == SetupType::Simulator) return "Simulator";

	Log_Print(LogLevel::Err, fmt::format("can not find setupType = {}", int(setupType)));
	exit(-1);
	return "";
}

SetupType CTransToStr::Get_SetupType(const std::string& setupType)
{
	if (setupType == "QGenerator") return SetupType::QGenerator;
	if (setupType == "RealTrader") return SetupType::RealTrader;
	if (setupType == "Simulator") return SetupType::Simulator;

	Log_Print(LogLevel::Err, fmt::format("can not find setupType = {}", setupType.c_str()));
	exit(-1);
	return SetupType::QGenerator;

}

std::string CTransToStr::Get_CurrentTimeType(CurrentTimeType currentTimeType)
{
	if (currentTimeType == CurrentTimeType::For_Real) return "For_Real";
	if (currentTimeType == CurrentTimeType::For_Simulator) return "For_Simulator";

	Log_Print(LogLevel::Err, fmt::format("can not find currentTimeType = {}", int(currentTimeType)));
	exit(-1);
	return "";
}

CurrentTimeType CTransToStr::Get_CurrentTimeType(const std::string& currentTimeType)
{
	if (currentTimeType == "For_Real") return CurrentTimeType::For_Real;
	if (currentTimeType == "For_Simulator") return CurrentTimeType::For_Simulator;

	Log_Print(LogLevel::Err, fmt::format("can not find currentTimeType = {}", currentTimeType.c_str()));
	exit(-1);
	return CurrentTimeType::For_Real;

}

std::string CTransToStr::Get_Thread_Status(Thread_Status thread_Status)
{
	if (thread_Status == Thread_Status::Running) return "Running";
	if (thread_Status == Thread_Status::Stop) return "Stop";

	Log_Print(LogLevel::Err, fmt::format("can not find thread_Status = {}", int(thread_Status)));
	exit(-1);
	return "";
}

Thread_Status CTransToStr::Get_Thread_Status(const std::string& thread_Status)
{
	if (thread_Status == "Running") return Thread_Status::Running;
	if (thread_Status == "Stop") return Thread_Status::Stop;

	Log_Print(LogLevel::Err, fmt::format("can not find thread_Status = {}", thread_Status.c_str()));
	exit(-1);
	return Thread_Status::Running;

}

std::string CTransToStr::Get_BuyOrSell(BuyOrSell buyOrSell)
{
	if (buyOrSell == BuyOrSell::Buy) return "Buy";

	return "Sell";
}

BuyOrSell CTransToStr::Get_BuyOrSell(const std::string& buyOrSell)
{
	if (buyOrSell == "Buy") return BuyOrSell::Buy;

	return BuyOrSell::Sell;
}

std::string CTransToStr::Get_RoundAction(RoundAction roundAction)
{
	if (roundAction == RoundAction::Open_Round) return "Open_Round";

	return "Cover_Round";
}

RoundAction CTransToStr::Get_RoundAction(const std::string& roundAction)
{
	if (roundAction == "Open_Round") return RoundAction::Open_Round;

	return RoundAction::Cover_Round;
}

std::string CTransToStr::Get_LongOrShort(LongOrShort longOrShort)
{
	if (longOrShort == LongOrShort::LongT) return "LongT";

	return "ShortT";
}

LongOrShort CTransToStr::Get_LongOrShort(const std::string& longOrShort)
{
	if (longOrShort == "LongT") return LongOrShort::LongT;

	return LongOrShort::ShortT;
}

std::string CTransToStr::Get_OrderStatus(OrderStatus orderStatus)
{
	if (orderStatus == OrderStatus::SomeDealed) return "SomeDealed";
	if (orderStatus == OrderStatus::AllCanceled) return "AllCanceled";
	if (orderStatus == OrderStatus::AllDealed) return "AllDealed";

	Log_Print(LogLevel::Err, fmt::format("can not find OrderStatus = {}", int(orderStatus)));
	exit(-1);
	return "";
}

OrderStatus CTransToStr::Get_OrderStatus(const std::string& orderStatus)
{
	if (orderStatus == "SomeDealed") return OrderStatus::SomeDealed;
	if (orderStatus == "AllCanceled") return OrderStatus::AllCanceled;
	if (orderStatus == "AllDealed") return OrderStatus::AllDealed;

	Log_Print(LogLevel::Err, fmt::format("can not find OrderStatus = {}", orderStatus.c_str()));
	exit(-1);
	return OrderStatus::SomeDealed;
}

std::string CTransToStr::Get_StSubModule(StSubModule stSubModule)
{
	if (stSubModule == StSubModule::SubOpen) return "SubOpen";

	return "SubCover";
}

StSubModule CTransToStr::Get_StSubModule(const std::string& stSubModule)
{
	if (stSubModule == "SubOpen") return StSubModule::SubOpen;

	return StSubModule::SubCover;
}

std::string CTransToStr::Get_TradePointStatus(TradePointStatus tradePointStatus)
{
	if (tradePointStatus == TradePointStatus::OpenMorning)
	{
		return "OpenMorning";
	}
	if (tradePointStatus == TradePointStatus::CloseMorning)
	{
		return "CloseMorning";
	}
	if (tradePointStatus == TradePointStatus::OpenAfternoon)
	{
		return "OpenAfternoon";
	}
	if (tradePointStatus == TradePointStatus::CloseAfternoon)
	{
		return "CloseAfternoon";
	}
	if (tradePointStatus == TradePointStatus::OpenEvening)
	{
		return "OpenEvening";
	}
	if (tradePointStatus == TradePointStatus::CloseEvening)
	{
		return "CloseEvening";
	}
	if (tradePointStatus == TradePointStatus::BreakBegin)
	{
		return "BreakBegin";
	}
	if (tradePointStatus == TradePointStatus::BreakEnd)
	{
		return "BreakEnd";
	}

	Log_Print(LogLevel::Err, fmt::format("can not find TradePointStatus: {}", int(tradePointStatus)));
	exit(-1);
	return "";


}

TradePointStatus CTransToStr::Get_TradePointStatus(const std::string& tradePointStatus)
{
	if (tradePointStatus == "OpenMorning")
	{
		return TradePointStatus::OpenMorning;
	}
	if (tradePointStatus == "CloseMorning")
	{
		return TradePointStatus::CloseMorning;
	}
	if (tradePointStatus == "OpenAfternoon")
	{
		return TradePointStatus::OpenAfternoon;
	}
	if (tradePointStatus == "CloseAfternoon")
	{
		return TradePointStatus::CloseAfternoon;
	}
	if (tradePointStatus == "OpenEvening")
	{
		return TradePointStatus::OpenEvening;
	}
	if (tradePointStatus == "CloseEvening")
	{
		return TradePointStatus::CloseEvening;
	}
	if (tradePointStatus == "BreakBegin")
	{
		return TradePointStatus::BreakBegin;
	}
	if (tradePointStatus == "BreakEnd")
	{
		return TradePointStatus::BreakEnd;
	}

	Log_Print(LogLevel::Err, fmt::format("can not find TradePointStatus: {}", tradePointStatus.c_str()));
	exit(-1);
	return TradePointStatus::BreakEnd;

}

std::string CTransToStr::Get_PriceStatus(PriceStatus priceStatus)
{
	if (priceStatus == PriceStatus::MakeItDeal) return "MakeItDeal";
	if (priceStatus == PriceStatus::MakeItPending) return "MakeItPending";

	Log_Print(LogLevel::Err, fmt::format("can not find priceStatus: {}", int(priceStatus)));
	exit(-1);
	return "";
}

PriceStatus CTransToStr::Get_PriceStatus(const std::string& priceStatus)
{
	if (priceStatus == "MakeItDeal") return PriceStatus::MakeItDeal;
	if (priceStatus == "MakeItPending") return PriceStatus::MakeItPending;

	Log_Print(LogLevel::Err, fmt::format("can not find priceStatus: {}", priceStatus.c_str()));
	exit(-1);
	return PriceStatus::MakeItDeal;
}

std::string CTransToStr::Get_TimeType(Time_Type timetype)
{
	std::string back = "";
	switch (timetype)
	{
	case Time_Type::S15:
		back = "S15";
		break;
	case Time_Type::M1:
		back = "M1";
		break;
	case Time_Type::M5:
		back = "M5";
		break;
	case Time_Type::M15:
		back = "M15";
		break;
	case Time_Type::M30:
		back = "M30";
		break;
	case Time_Type::H1:
		back = "H1";
		break;
	case Time_Type::D1:
		back = "D1";
		break;
	default:
		printf("unknown timetype \n");
		exit(-1);
		break;
	}
	return back;
}

Time_Type CTransToStr::Get_TimeType(const std::string& timetype)
{
	if (timetype == "S15") return Time_Type::S15;
	if (timetype == "M1") return Time_Type::M1;
	if (timetype == "M5") return Time_Type::M5;
	if (timetype == "M15") return Time_Type::M15;
	if (timetype == "M30") return Time_Type::M30;
	if (timetype == "H1") return Time_Type::H1;
	if (timetype == "D1") return Time_Type::D1;

	printf("unknown timetypestr = %s \n", timetype.c_str());
	exit(-1);
	return Time_Type::M1;
}



std::string CTransToStr::Get_LineType(LineType linetype)
{
	if (linetype == LineType::UseTick) return "UseTick";
	if (linetype == LineType::UseKLine) return "UseKLine";
	if (linetype == LineType::UseMa) return "UseMa";

	printf("unknown lineType \n");
	exit(-1);
	return "";
}

LineType CTransToStr::Get_LineType(const std::string& linetype)
{
	if (linetype == "UseTick")  return LineType::UseTick;
	if (linetype == "UseKLine")  return LineType::UseKLine;
	if (linetype == "UseMa")  return LineType::UseMa;

	printf("unknown linetype: %s \n", linetype.c_str());
	exit(-1);
	return LineType::UseMa;
}


std::string CTransToStr::Get_ActiveOrInactive(ActiveOrInactive activeOrInactive)
{
	if (activeOrInactive == ActiveOrInactive::Active) return "Active";
	if (activeOrInactive == ActiveOrInactive::Inactive) return "Inactive";

	printf("unknown activeOrInactive \n");
	exit(-1);
	return "";
}

ActiveOrInactive CTransToStr::Get_ActiveOrInactive(const std::string& activeOrInactive)
{
	if (activeOrInactive == "Active") return ActiveOrInactive::Active;
	if (activeOrInactive == "Inactive") return ActiveOrInactive::Inactive;

	printf("unknown activeOrInactive: %s \n", activeOrInactive.c_str());
	exit(-1);
	return ActiveOrInactive::Active;
}

std::string CTransToStr::Get_MarketType(MarketType marketType)
{
	if (marketType == MarketType::PreMarket) return "PreMarket";
	if (marketType == MarketType::AfterHour) return "AfterHour";
	if (marketType == MarketType::RegularMarket) return "RegularMarket";

	printf("unknown marketType \n");
	exit(-1);
	return "";
}

MarketType CTransToStr::Get_MarketType(const std::string& marketType)
{
	if (marketType == "PreMarket") return MarketType::PreMarket;
	if (marketType == "AfterHour") return MarketType::AfterHour;
	if (marketType == "RegularMarket") return MarketType::RegularMarket;

	printf("unknown marketType: %s \n", marketType.c_str());
	exit(-1);
	return MarketType::PreMarket;
}

std::string CTransToStr::Get_TrendType(TrendType trendType)
{
	if (trendType == TrendType::AntiTrend)
	{
		return "AntiTrend";
	}
	else
	{
		return "ProTrend";
	}


}

TrendType CTransToStr::Get_TrendType(const std::string& trendType)
{
	return TrendType();
}

std::string CTransToStr::Get_DivergenceType(DivergenceType divergenceType)
{
	std::string back = "";
	switch (divergenceType)
	{
	case DivergenceType::Normal:
		back = "Normal";
		break;
	case DivergenceType::Top:
		back = "Top";
		break;
	case DivergenceType::Bottom:
		back = "Bottom";
		break;
	case DivergenceType::TopSub:
		back = "TopSub";
		break;
	case DivergenceType::BottomSub:
		back = "BottomSub";
		break;
	default:
		break;
	}

	return back;
}

DivergenceType CTransToStr::Get_DivergenceType(const std::string& divergenceType)
{
	if (divergenceType == "Normal") return DivergenceType::Normal;
	if (divergenceType == "Top") return DivergenceType::Top;
	if (divergenceType == "Bottom") return DivergenceType::Bottom;
	if (divergenceType == "TopSub") return DivergenceType::TopSub;
	if (divergenceType == "BottomSub") return DivergenceType::BottomSub;
	Log_Print(LogLevel::Err, fmt::format("can not find divergenceType: {}", divergenceType.c_str()));
	exit(-1);
	return DivergenceType::Normal;
	
}

std::string CTransToStr::TransToLowerCaseOrUpperCase(const std::string& strIn, UpperOrLower upperOrLower)
{
	std::string back;
	if (upperOrLower == UpperOrLower::UpperCase)
	{
		transform(strIn.begin(), strIn.end(), back_inserter(back), ::toupper);
	}
	else
	{
		transform(strIn.begin(), strIn.end(), back_inserter(back), ::tolower);
	}
	return back;

}
