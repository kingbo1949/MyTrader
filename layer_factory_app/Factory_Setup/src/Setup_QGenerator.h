#pragma once
#include "Setup_Real.h"
class CSetup_QGenerator : public CSetup_Real
{
public:
	CSetup_QGenerator();
	virtual ~CSetup_QGenerator() { ; };

protected:
	// 初始化策略
	virtual void				Init_Factory_Strategys() override final;

	// 启动守护线程
	virtual void				Init_DaemonObject() override final;

	// 初始化行情回调对象
	virtual void				Init_QCallbackObject() override final;


	virtual void				Init_Factory_TShareEnv() override final;

	virtual void				Init_Factory_QShareEnv() override final;

	virtual void				Init_Factory_StrategyHashEnv() override final;

	virtual void				Init_Factory_UnifyInterface() override final;

	// 启动api远程连接
	virtual void				RunIBApi() override final;

	// 查询并更新最近两天K线到数据库
	void						QueryAndUpdateKline(IbContractPtrs contracts);

	// 查询并更新最近两天K线到数据库
	void						QueryAndUpdateKline(const CodeStr& codeId, Time_Type timeType);

	// 删除K线
	void						RemoveKlines(const CodeStr& codeId, Time_Type timeType, const std::string& beginTimestr, const std::string& endTimeStr);

	// 判断品种是否需要订阅 不订阅的品种仅每天更新数据而不产生数据库行情压力
	bool						NeedSubscribe(const CodeStr& codeId);






};

