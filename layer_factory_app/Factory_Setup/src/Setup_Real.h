#pragma once
#include "Setup.h"
class CSetup_Real : public CSetup
{
public:
	CSetup_Real(CurrentTimeType	currentType);
	virtual ~CSetup_Real() { ; };

	virtual void				Open() override final;

	// 关闭
	virtual void				Close() override final;

protected:
	CurrentTimeType				m_currentType;

	// 初始化守护对象
	virtual void				Init_DaemonObject() = 0;

	// 初始化行情回调对象
	virtual void				Init_QCallbackObject() = 0;

	// 初始化策略
	virtual void				Init_Factory_Strategys() = 0;

	virtual void				Init_Factory_TShareEnv() = 0;

	virtual void				Init_Factory_StrategyHashEnv() = 0;

	virtual void				Init_Factory_QShareEnv() = 0;

	virtual void				Init_Factory_UnifyInterface() = 0;


	// 启动api远程连接
	virtual void				RunIBApi() = 0;

	// 注册行情接收对象
	void						RegisterQCallbackObject() ;

	// 注册交易回报接收对象(只有实盘需要接收交易回报)
	virtual void				RegisterTCallbackObject() { ; };

	void						Init_Factory_Global();

	void						Init_Factory_CodeIdHashEnv();

	void						Init_Factory_Log();

	void						Init_Factory_QDatabase();

	

	void						Init_Factory_Topics();



	
	

	

};

