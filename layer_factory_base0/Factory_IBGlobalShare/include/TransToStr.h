#pragma once
#include "Factory_IBGlobalShare_Export.h"


#include <base_struc.h>
#include <base_trade.h>
#include <string>

enum class UpperOrLower
{
	UpperCase = 0,		// 大写
	LowerCase = 1		// 小写
};


class FACTORY_IBGLOBALSHARE_API CTransToStr
{
public:
	CTransToStr();
	virtual ~CTransToStr();

	static std::string					Get_Bool(bool trueOrFalse);
	static bool							Get_Bool(const std::string& trueOrFalse);

	static std::string					Get_SecurityType(SecurityType securityType);
	static SecurityType					Get_SecurityType(const std::string& SecurityTypeStr);


	static std::string					Get_ExchangePl(ExchangePl exchangePl);
	static ExchangePl					Get_ExchangePl(const std::string& exchangePlStr);

	static std::string					Get_Money(Money money);
	static Money						Get_Money(const std::string& moneyStr);

	static std::string					Get_SetupType(SetupType setupType);
	static SetupType					Get_SetupType(const std::string& setupType);

	static std::string					Get_CurrentTimeType(CurrentTimeType currentTimeType);
	static CurrentTimeType				Get_CurrentTimeType(const std::string& currentTimeType);

	static std::string					Get_Thread_Status(Thread_Status thread_Status);
	static Thread_Status				Get_Thread_Status(const std::string& thread_Status);

	static std::string					Get_BuyOrSell(BuyOrSell buyOrSell);
	static BuyOrSell					Get_BuyOrSell(const std::string& buyOrSell);

	static std::string					Get_RoundAction(RoundAction roundAction);
	static RoundAction					Get_RoundAction(const std::string& roundAction);

	static std::string					Get_LongOrShort(LongOrShort longOrShort);
	static LongOrShort					Get_LongOrShort(const std::string& longOrShort);

	static std::string					Get_OrderStatus(OrderStatus orderStatus);
	static OrderStatus					Get_OrderStatus(const std::string& orderStatus);

	static std::string					Get_StSubModule(StSubModule stSubModule);
	static StSubModule					Get_StSubModule(const std::string& stSubModule);

	static std::string					Get_TradePointStatus(TradePointStatus tradePointStatus);
	static TradePointStatus				Get_TradePointStatus(const std::string& tradePointStatus);

	static std::string					Get_PriceStatus(PriceStatus priceStatus);
	static PriceStatus					Get_PriceStatus(const std::string& priceStatus);

	static std::string					Get_TimeType(Time_Type timetype);
	static Time_Type					Get_TimeType(const std::string& timetype);

	static std::string					Get_LineType(LineType linetype);
	static LineType						Get_LineType(const std::string& linetype);


	static std::string					Get_ActiveOrInactive(ActiveOrInactive activeOrInactive);
	static ActiveOrInactive				Get_ActiveOrInactive(const std::string& activeOrInactive);

	static std::string					Get_MarketType(MarketType marketType);
	static MarketType					Get_MarketType(const std::string& marketType);

	static std::string					Get_TrendType(TrendType trendType);
	static TrendType					Get_TrendType(const std::string& trendType);

	static std::string					Get_DivergenceType(DivergenceType divergenceType);
	DivergenceType						Get_DivergenceType(const std::string& divergenceType);

	// 字符串大小写转换
	static std::string					TransToLowerCaseOrUpperCase(const std::string& strIn, UpperOrLower upperOrLower);





};

