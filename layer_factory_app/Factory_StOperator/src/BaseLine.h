#pragma once
#include <base_struc.h>
#include <base_trade.h>

// 为应用层提供PricePairPtr对象，作为开平仓条件
// 同一个策略的开平仓模块共享同一个CBaseLine对象实例
// PricePairPtr的开仓条件是动态的，随行情不同而不断改变，一旦开仓之后，其平仓条件就不再变了。



class CBaseLine
{
public:
	CBaseLine(StrategyParamPtr pStrategyParam, IbContractPtr pContract, time_t openPoint);
	virtual ~CBaseLine() { ; };

	// 返回开仓基准线 
	PricePairPtr					MakeAndGetLine(time_t now, RoundAction roundAction);

protected:
	StrategyIdHashId				m_strategyIdHashId;
	StrategyParamPtr				m_pStrategyParam;
	IbContractPtr					m_pContract;
	time_t							m_openPoint;// 可以开始开仓的当日第一根K线时间

	PricePairPtr					m_pricePair;// 开平仓条件

	CodeStr							GetCodeId();
	Time_Type						GetTimeType();

	
	// 按动态行情重新刷新m_pricePair的值
	virtual bool					FreshPricePair(time_t now) = 0;

	// m_pricePair已经存在，判断是否需要调整刷新
	virtual bool					NeedFreshPricePair() = 0;

	virtual double					TransParam(double param) = 0;

	

};
typedef std::shared_ptr<CBaseLine> BaseLinePtr;
