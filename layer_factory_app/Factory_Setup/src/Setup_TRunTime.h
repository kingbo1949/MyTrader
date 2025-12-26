#pragma once
#include "Setup_Real.h"
class CSetup_TRunTime : public CSetup_Real
{
public:
	CSetup_TRunTime();
	virtual ~CSetup_TRunTime() { ; };

protected:
	// 初始化策略
	virtual void				Init_Factory_Strategys() override final;

	virtual void				RegisterTCallbackObject() override final;

	// 初始化守护对象
	virtual void				Init_DaemonObject() override final;

	// 初始化行情回调对象
	virtual void				Init_QCallbackObject() override final;

	virtual void				Init_Factory_TShareEnv() override final;

	virtual void				Init_Factory_QShareEnv() override final;

	virtual void				Init_Factory_StrategyHashEnv() override final;

	virtual void				Init_Factory_UnifyInterface() override final;

	// 启动api远程连接
	virtual void				RunIBApi() override final;

	// 从文件装载LastOrder
	void						LoadLastOrderFromFile();

	// 从文件装载PricePair
	void						LoadPricePairFromFile();
};

