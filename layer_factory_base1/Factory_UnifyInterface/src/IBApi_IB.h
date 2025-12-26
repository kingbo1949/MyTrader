#pragma once
#include "IBApi_Real.h"
class CIBApi_IB : public CIBApi_Real
{
public:
	CIBApi_IB();
	virtual ~CIBApi_IB();

	/// 建立连接
	virtual void				Connect()  override final;

	///查询tick, 从起始点往后查询count个tick数据
	virtual IBTickPtrs			QueryTicks(const CodeStr& codeId, time_t beginSecond, int count) override final;


	///查询tick, 只能查询盘前或者盘后，daySecond是当日零点
	virtual IBTickPtrs			QueryTicks(const CodeStr& codeId, time_t daySecond, MarketType marketType) override final;

	/// 查询k线
	/// IB接口是倒序查询，给出结尾时点（endSecond），然后往前查days天
	/// IB接口期货一旦指定CONFUT就是查询连续合约，无论在localSymbol指定NQU4还是NQM4都不再有意义
	/// 因此StLoader和QuoteGenerator使用RealTrader查询，这样实盘中就可以使用NQU4还是NQM4
	/// 而QTransKLineFormIB使用Simulator指定CONFUT,下载数据用于测试
	/// 虽然每天实盘交易会更新数据库污染测试数据，但是等连续合约到期之后，重新执行QTransKLineFormIB更新数据即可
	virtual IBKLinePtrs			QueryKLine(const CodeStr& codeId, Time_Type timetype, time_t endSecond, int days, bool onlyRegularTime, UseType useType) override final;

	/// 查询k线
	/// IB接口是倒序查询，给出结尾时点（endSecond），然后往前查days天，一路接力查询直到越过beginSecond为止
	virtual IBKLinePtrs			QueryKLineRange(const CodeStr& codeId, Time_Type timetype, Second_T beginSecond, Second_T endSecond, int daysForMinute, bool onlyRegularTime, UseType useType) override final;

	/// 订阅行情
	virtual void				SubscribeQuote(const IbContractPtrs& contracts)  override final;

	/// 取消订阅行情
	virtual void				UnSubscribeQuote(const IbContractPtrs& contracts) override final;

	/// 报单录入请求
	virtual bool				SendOrder(OneOrderPtr porder)  override final;


	///报单操作请求（撤单）
	virtual bool				CancelOrder(OneOrderPtr porder, const std::string& manualOrderCancelTime)  override final;

protected:
	EReaderOSSignal				m_osSignal;
	CIBSpi						m_spi;
	EClientSocket* const		m_pApi;
	EReader* m_pReader;
	Cmd_IBDaemonPtr				m_cmd;

	Contract					Trans(IbContractPtr pcontract);

	Contract					Trans(CodeHashId codeHash);

	Order						TransToIbOrder(OneOrderPtr porder);

	std::string					TransToBarSize(Time_Type timetype);

	// 把k线时间调整到合规的recordNo
	IBKLinePtrs					ChgTimeToRecordeNo(const IBKLinePtrs& klines, Time_Type timeType);

	// 指定时间内try一个连接
	bool						TryConnect(time_t millisecond);

	// 输入美国东部时间，得到IB查询时间字符串，需要添加时区
	std::string					GetIBQueryTimeStr(IbContractPtr contract, time_t timeSecond);

};

