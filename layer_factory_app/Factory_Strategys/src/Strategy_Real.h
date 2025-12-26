#pragma once
#include "StrategyBase.h"
#include <base_struc.h>
#include <base_trade.h>
#include <Factory_QShareEnv.h>
#include <Factory_StOperator.h>

// 处理流程关键：
// 策略模块有活跃未成交单，则处于DoSend状态，否则处于DoAtFirst态
// MakeOrderVol()函数会检查持仓 :
//		如果持仓为空，则CStrategy_Cover不执行, 开仓模块执行
//		如果持仓半满，则CStrategy_Cover执行,   开仓模块执行
//		如果持仓为满，则CStrategy_Cover执行,   开仓模块不执行
//		混合品种的委托在应用层永远只有1手，没有半满仓这个状态，具体合约的手数由底层决定

enum class StModuleStatus
{
	DoAtFirst,			// 就绪态，准备发送委托，这个状态中没有活跃挂单
	DoSend				// 已发送，这个状态有活跃挂单，并且成交也在变化，没有活跃挂单之后状态转到DoAtFirst态
};


class CStrategy_Real : public CStrategyBase
{
public:
	CStrategy_Real(StrategyParamPtr	strategyParam);
	virtual ~CStrategy_Real() { ; };

	virtual void						Init() override final;

	// 设置初始参数
	virtual void						SetTradeParams() = 0;

	// 行情触发
	virtual void						OnRtnTick(CodeHashId codeHash, RoundAction openOrCover) override final;

	///交易回报
	virtual void						OnRtnTrade(StrategyIdHashId strategyIdHashId) override final;

	

protected:
	std::mutex							m_mutex;
	StrategyParamPtr					m_strategyParam;
	SubModuleParams						m_stParams;
	TickEnvPtr							m_tickEnv;

	MakePricePtr						m_pMakePrice;
	NeedCancelPtr						m_pNeedCancel;


	

	// 主执行流程
	void 								Execute();
	void 								Execute_DoAtFirst();
	void 								Execute_DoSend();


	StModuleStatus						GetStModuleStatus();

	StHalfStatus						GetStHalfStatus();


	// 策略计算下单价格
	bool								MakeOrderPrice(double& price, RealPriceType& priceType);

	// 按照priceType把策略价格转换成真实价格
	double								MakeSendPrice(double priceStrategy, RealPriceType priceType);

	// 是否需要撤单
	bool								NeedCancel();

	// 计算下单手数
	virtual double						MakeOrderVol() = 0;

	// 制作委托
	OneOrderPtr							MakeOrder(double vol, double priceStrategy, double priceReal);

	// 发送委托
	bool								SendOrder(OneOrderPtr porder);

	// 发送撤单
	bool								CancelOrder(OneOrderPtr porder);

	// 检查TickEnvPtr环境
	bool								CheckTickEnv();

	void								PrintPriceToLog(double strategyPrice, double realSendPrice);
};

