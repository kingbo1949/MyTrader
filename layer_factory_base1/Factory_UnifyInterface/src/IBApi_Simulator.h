#pragma once
#include "IBApi_Real.h"
class CIBApi_Simulator : public CIBApi_Real
{
public:
	CIBApi_Simulator() { ; };
	virtual ~CIBApi_Simulator() { ; };

	///建立连接
	virtual void				Connect() override final { ; };

	///查询tick, 从起始点往后查询count个tick数据
	virtual IBTickPtrs			QueryTicks(const CodeStr& codeId, time_t beginSecond, int count) override final;


	///查询tick, 只能查询盘前或者盘后，daySecond是当日零点
	virtual IBTickPtrs			QueryTicks(const CodeStr& codeId, time_t daySecond, MarketType marketType) override final;

	///查询k线
	virtual IBKLinePtrs			QueryKLine(const CodeStr& codeId, Time_Type timetype, time_t endSecond, int days, bool onlyRegularTime, UseType useType) override final;

	///查询k线
	virtual IBKLinePtrs			QueryKLineRange(const CodeStr& codeId, Time_Type timetype, time_t beginSecond, time_t endSecond, int daysForMinute, bool onlyRegularTime, UseType useType) override final;

	///订阅行情
	virtual void				SubscribeQuote(const IbContractPtrs& contracts) override final { ; };

	///取消订阅行情
	virtual void				UnSubscribeQuote(const IbContractPtrs& contracts) override final { ; };

	///报单录入请求
	virtual bool				SendOrder(OneOrderPtr porder)  override final;


	///报单操作请求（撤单）
	virtual bool				CancelOrder(OneOrderPtr porder, const std::string& manualOrderCancelTime)  override final;

};

