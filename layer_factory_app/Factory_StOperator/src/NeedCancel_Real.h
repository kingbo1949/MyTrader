#pragma once
#include "NeedCancel.h"
#include <Factory_QShareEnv.h>
class CNeedCancel_Real : public CNeedCancel
{
public:
	CNeedCancel_Real(const SubModuleParams& stParams);
	virtual ~CNeedCancel_Real() { ; };

	virtual bool			CanCancel() override final;

protected:
	SubModuleParams			m_stParams;
	StrategyParamPtr		m_pStrategyParam;
	TickEnvPtr				m_targetCodeIdEnv;

	virtual bool			CanCancelOpen() = 0;

	virtual bool			CanCancelCover() = 0;

	// 检测是否挂单在3秒以内，或者处于最前面
	bool					CheckRecentOrderOnTop(int second);



};

