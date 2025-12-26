#pragma once
#include "IBApi.h"
#include <base_struc.h>
#include <EClientSocket.h>
#include <EReaderOSSignal.h>
#include <EReader.h>
#include "Cmd_IBDaemon.h"
#
#include "IBSpi.h"

class CIBApi_Real : public CIBApi
{
public:
	CIBApi_Real() { ; };
	virtual ~CIBApi_Real() { ; };

	///初始化
	///@remark 初始化运行环境，只有调用后,接口才开始工作
	virtual void				Init() override final { ; };

	///删除接口对象本身
	///@remark 不再使用本接口对象时,调用该函数删除接口对象
	virtual void				Release()  override final ;

	///建立连接
	virtual void				Connect()  = 0;

	///查询tick, 从起始点往后查询count个tick数据
	virtual IBTickPtrs			QueryTicks(const CodeStr& codeId, time_t beginSecond, int count) = 0;

	///查询tick, 只能查询盘前或者盘后，daySecond是当日零点
	virtual IBTickPtrs			QueryTicks(const CodeStr& codeId, time_t daySecond, MarketType marketType) = 0;

	///查询k线
	virtual IBKLinePtrs			QueryKLine(const CodeStr& codeId, Time_Type timetype, time_t endSecond, int days, bool onlyRegularTime, UseType useType) = 0;

	///查询k线
	virtual IBKLinePtrs			QueryKLineRange(const CodeStr& codeId, Time_Type timetype, time_t beginSecond, time_t endSecond, int daysForMinute, bool onlyRegularTime, UseType useType) = 0;

	///订阅行情
	virtual void				SubscribeQuote(const IbContractPtrs& contracts) = 0;

	///取消订阅行情
	virtual void				UnSubscribeQuote(const IbContractPtrs& contracts) = 0;

	///报单录入请求
	virtual bool				SendOrder(OneOrderPtr porder) override;


	///报单操作请求（撤单）
	virtual bool				CancelOrder(OneOrderPtr porder, const std::string& manualOrderCancelTime) override;

protected:


};

