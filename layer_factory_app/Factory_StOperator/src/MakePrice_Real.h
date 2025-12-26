#pragma once
#include "MakePrice.h"
#include <Factory_QShareEnv.h>
#include "Factory_StOperator.h"
class CMakePrice_Real : public CMakePrice
{
public:
	CMakePrice_Real(const SubModuleParams& stParams);
	virtual ~CMakePrice_Real() { ; };

	// 返回下单的价位， 为零则不下单
	virtual bool			GetPrice(double& price, RealPriceType& priceType) override final;

protected:
	SubModuleParams			m_stParams;
	StrategyParamPtr		m_pStrategyParam;
	TickEnvPtr				m_targetTickEnv;
	TickEnvPtr				m_targetLastTickEnv;

	// 临近收盘是否可以发单，缺省实现是可以
	virtual bool			CanMakePriceNearClose() override { return true; };

	virtual bool			GetOpenPrice(double& price, RealPriceType& priceType) = 0;

	virtual bool			GetCoverPrice(double& price, RealPriceType& priceType) = 0;


	// 制作价格的预处理
	bool					PreGetPrice();

	// 取得反向持仓
	PositionSizePtr			GetOppositPs();

	// 取得最近成交的开仓单
	OneOrderPtr				GetRecentDealOpenOrder();

	//
	CodeStr					GetCodeId();

	Time_Type				GetTimeType();


	// 返回当前时间 毫秒
	time_t					Now();

	// 返回当前时间的一分钟时间标 毫秒
	time_t					NowM1();

	// 返回当前时间的30分钟时间标 毫秒
	Tick_T					NowM30();

	// 返回上一个30分钟时间标 毫秒
	Tick_T					LastM30();



	std::string				GetTickTimeStr(time_t ticktime);

};

