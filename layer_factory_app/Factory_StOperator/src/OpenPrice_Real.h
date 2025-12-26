#pragma once
#include "OpenPrice.h"
class COpenPrice_Real : public COpenPrice
{
public:
	COpenPrice_Real() { ; };
	virtual ~COpenPrice_Real() { ; };

	virtual bool			GetOpenPrice(StrategyIdHashId strategyIdHashId, double& price) override final;

	virtual void			SetOpenPrice(StrategyIdHashId strategyIdHashId, double price)  override final;

	virtual void			DelOpenPrice(StrategyIdHashId strategyIdHashId)  override final;

protected:
	std::mutex							m_mutex;
	std::map<StrategyIdHashId, double>	m_openPrices;

};

