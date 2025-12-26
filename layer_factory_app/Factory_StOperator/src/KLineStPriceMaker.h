#pragma once
#include <base_trade.h>
#include "KLineAndMaTool.h"
class CKLineStPriceMaker
{
public:
	CKLineStPriceMaker(StrategyParamPtr	pStrategyParam, const SubModuleParams& stParams, IBKLinePtr thisBar, IBKLinePtr lastBar);
	virtual ~CKLineStPriceMaker() { ; };

	bool				Make(double& price);

protected:
	StrategyParamPtr	m_pStrategyParam;
	SubModuleParams		m_stParams;
	IBKLinePtr			m_thisKline = nullptr;	// 当前BAR
	IBKLinePtr			m_lastKline = nullptr;	// 上一个根BAR
	CodeStr				m_codeId;
	Time_Type			m_timetype;
	time_t				m_now;
	HighAndLow			m_highAndLow ;			// 20 根K线的极值
	double				m_diff = 0;				// 对极值m_extremeValue的偏离
	double				m_extremeValue = 0;		// 最后计算出来的极值
	
	// 确定m_diffstep的值
	virtual void		MakeDiffStep() = 0;

	// 确定m_highAndLow的值
	virtual bool		MakeHighAndLow();

	// 确定最后极值m_extremeValue
	virtual void		MakeExtremeValue() = 0;

	// 对m_extremeValue做偏离处理得到返回值
	bool				MakePriceByExtremePrice(double& price) ;

	// 制作委托价格失败，通知Monitor失败的原因
	virtual void		UpdateMonitor() = 0;
};
typedef std::shared_ptr<CKLineStPriceMaker> KLineStPriceMakerPtr;