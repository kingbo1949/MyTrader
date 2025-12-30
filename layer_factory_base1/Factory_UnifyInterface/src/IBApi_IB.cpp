#include "pch.h"
#include "IBApi_IB.h"

#include <Factory_Log.h>
#include <Factory_IBJSon.h>
#include <Factory_HashEnv.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_TShareEnv.h>
#include <Factory_QDatabase.h>
#include <Global.h>
#include <base_struc.h>
#include "Factory_UnifyInterface.h"
// #include <IntelRDFPMathLib.h>


CIBApi_IB::CIBApi_IB()
	:m_osSignal(2000)//2-seconds timeout
	, m_pApi(new EClientSocket(&m_spi, &m_osSignal))
{
	m_spi.SetApi(m_pApi);
}

CIBApi_IB::~CIBApi_IB()
{
	if (m_pReader)
	{
		delete m_pReader;
	}
	if (m_pApi)
	{
		delete m_pApi;
	}

}

void CIBApi_IB::Connect()
{
	bool success = TryConnect(5000);
	if (!success)
	{
		Log_Print(LogLevel::Err, fmt::format("try some times, can not connect to TWS or gateway"));
		exit(-1);
	}

	m_pReader = new EReader(m_pApi, &m_osSignal);
	m_pReader->start();

	// 启动接收线程
	m_cmd = std::make_shared<CCmd_IBDaemon>(m_pApi, m_pReader, &m_osSignal);
	MakeAndGet_MyThreadPool()->commit(*m_cmd);

	return;

}

IBTickPtrs CIBApi_IB::QueryTicks(const CodeStr& codeId, time_t daySecond, MarketType marketType)
{
	if (marketType == MarketType::RegularMarket)
	{
		Log_Print(LogLevel::Critical, "QueryTicks can not use in Regular hour market");
		exit(-1);
	}
	// 确定起始和终止时间
	time_t beginSecond = 0;
	time_t endSecond = 0;
	if (marketType == MarketType::PreMarket)
	{
		// 04:00 - 09:30
		beginSecond = daySecond + 4 * 3600;
		endSecond = daySecond + 9 * 3600 + 30 * 60;
	}
	else
	{
		// 16:00 - 20:00
		beginSecond = daySecond + 16 * 3600;
		endSecond = daySecond + 20 * 3600;
	}
	CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(codeId.c_str());
	MakeAndGet_TickBoard()->SetCodeHash(codeHash);

	IBTickPtrs back;
	time_t thisBeginSecond = beginSecond;
	while (true)
	{
		IBTickPtrs ticks = QueryTicks(codeId, thisBeginSecond, 1000);
		if (ticks.empty()) break;

		Log_Print(LogLevel::Info, fmt::format("{} begintime = {}, ticks size = {}, {}->{}",
			codeId.c_str(),
			CGlobal::GetTimeStr(thisBeginSecond).c_str(),
			ticks.size(),
			CGlobal::GetTickTimeStr(ticks[0]->time).c_str(),
			CGlobal::GetTickTimeStr(ticks.back()->time).c_str()
		));

		if (ticks.back()->time / 1000 < endSecond)
		{
			// 尚未找到头
			thisBeginSecond = ticks.back()->time / 1000 + 1;
			copy(ticks.begin(), ticks.end(), back_inserter(back));
			continue;
		}

		// 已经到头了
		IBTickPtrs tem;
		for (auto onetick : ticks)
		{
			if (onetick->time / 1000 < endSecond)
			{
				tem.push_back(onetick);
			}
		}
		copy(tem.begin(), tem.end(), back_inserter(back));
		break;
	}

	return back;



}

IBTickPtrs CIBApi_IB::QueryTicks(const CodeStr& codeId, time_t beginSecond, int count)
{
	CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(codeId.c_str());
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeHash);
	Contract ibContract = Trans(contract);

	IBTickPtrs back;
	if (MakeAndGet_TickBoard()->GetTickBoardStatus() != TickBoardStatus::Ready)
	{
		Log_Print(LogLevel::Warn, "TickBoardStatus != TickBoardStatus::Ready, can not queryTicks");
		return back;
	}
	std::string beginTimeStr = CGlobal::GetTimeStr(beginSecond);
	MakeAndGet_TickBoard()->SetTickBoardStatus(TickBoardStatus::Busy);


	// 如果起始时间处于盘前，则有可能返回盘中的数据
	// 但是如果起始时间处于盘后，则只能返回盘后数据，而不会返回第二天数据
	m_pApi->reqHistoricalTicks(codeHash, ibContract, beginTimeStr, "", count, "TRADES", 0, true, TagValueListSPtr());

	while (true)
	{
		if (MakeAndGet_TickBoard()->GetTickBoardStatus() == TickBoardStatus::Done) break;

		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
	back = MakeAndGet_TickBoard()->ReceiveAll();

	return back;

}

IBKLinePtrs CIBApi_IB::QueryKLine(const CodeStr& codeId, Time_Type timetype, time_t endSecond, int days, bool onlyRegularTime, UseType useType)
{
	CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(codeId.c_str());
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeHash);
	Contract ibContract = Trans(contract);
	if (useType == UseType::Simulator && contract->securityType == SecurityType::FUT)
	{
		// QTransKLineFromIB运行在Simulator模式，它只能查询连续合约
		ibContract.secType = "CONTFUT";
	}

	std::string endSecondStr = GetIBQueryTimeStr(contract, endSecond);
	//std::string endSecondStr = "20250201 00:00:00 US/Eastern";
	if (ibContract.secType == "CONTFUT")
	{
		// 连续合约只能查询到当前时间
		endSecondStr = "";
	}

	Log_Print(LogLevel::Warn, fmt::format("endSecondStr = {}", endSecondStr.c_str()));

	std::string durationStr = fmt::format("{} D", days);;

	std::string barSize = TransToBarSize(timetype);

	IBKLinePtrs back;
	if (MakeAndGet_KLineBoard()->GetKLineBoardStatus() != KLineBoardStatus::Ready)
	{
		Log_Print(LogLevel::Warn, "KLineBoardStatus != KLineBoardStatus::Ready, can not queryKline");
		return back;
	}

	MakeAndGet_KLineBoard()->SetKLineBoardStatus(KLineBoardStatus::Busy);
	if (onlyRegularTime)
	{
		// 仅常规开盘时间
		m_pApi->reqHistoricalData(codeHash, ibContract, endSecondStr, durationStr, barSize, "TRADES", 1, 1, false, TagValueListSPtr());
	}
	else
	{
		// 包含盘前盘后
		//endSecondStr = "";
		m_pApi->reqHistoricalData(codeHash, ibContract, endSecondStr, durationStr, barSize, "TRADES", 0, 1, false, TagValueListSPtr());
	}

	while (true)
	{
		if (MakeAndGet_KLineBoard()->GetKLineBoardStatus() == KLineBoardStatus::Done) break;

		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	back = ChgTimeToRecordeNo(MakeAndGet_KLineBoard()->ReceiveAll(), timetype);
	//for (auto kline : back)
	//{
	//	std::string str = CTransDataToStr::GetKlineStr_Csv(kline);
	//	Log_Print(LogLevel::Warn, fmt::format("{} ", str.c_str()));
	//}
	return back;
}

IBKLinePtrs CIBApi_IB::QueryKLineRange(const CodeStr& codeId, Time_Type timetype, Second_T beginSecond, Second_T endSecond, int daysForMinute, bool onlyRegularTime, UseType useType)
{
	IBKLinePtrs back;
	time_t thisEndSecond = endSecond;

	TimePair lastResultTimePair;
	while (true)
	{
		Log_Print(LogLevel::Warn, fmt::format("{} {} begin query in ib_api, endtime = {}, days = {}",
			codeId.c_str(),
			CTransToStr::Get_TimeType(timetype).c_str(),
			CGlobal::GetTimeStr(thisEndSecond).c_str(),
			daysForMinute
		));
		IBKLinePtrs klines = QueryKLine(codeId, timetype, thisEndSecond, daysForMinute, onlyRegularTime, useType);
		if (klines.empty())
		{
			break;
		}
		//CHighFrequencyGlobalFunc::PrintKLines(klines);


		TimePair thisResultTimePair;
		thisResultTimePair.beginPos = klines[0]->time;
		thisResultTimePair.endPos = klines.back()->time;

		Log_Print(LogLevel::Warn, fmt::format("{} {}   end query, endtime = {}, days = {}, result: {} - {}, kline size = {}",
			codeId.c_str(),
			CTransToStr::Get_TimeType(timetype).c_str(),
			CGlobal::GetTimeStr(thisEndSecond).c_str(),
			daysForMinute,
			CGlobal::GetTickTimeStr(klines[0]->time),
			CGlobal::GetTickTimeStr(klines.back()->time),
			klines.size()
		));
		if (thisResultTimePair.beginPos == lastResultTimePair.beginPos && thisResultTimePair.endPos == lastResultTimePair.endPos)
		{
			// IB 数据接口 查不动了就会总返回同样的数据
			break;
		}
		lastResultTimePair = thisResultTimePair;



		if (klines[0]->time / 1000 > beginSecond)
		{
			// 尚未找到头
			thisEndSecond = klines[0]->time / 1000 - 5;

			copy(back.begin(), back.end(), back_inserter(klines));
			klines.swap(back);
			continue;
		}

		// 已经到头了
		IBKLinePtrs tem;
		for (auto onekline : klines)
		{
			if (onekline->time / 1000 < beginSecond) continue;
			tem.push_back(onekline);
		}
		copy(back.begin(), back.end(), back_inserter(tem));
		tem.swap(back);
		break;
	}

	return back;
}

void CIBApi_IB::SubscribeQuote(const IbContractPtrs& contracts)
{
	for (auto one_contract : contracts)
	{
		//std::this_thread::sleep_for(std::chrono::seconds(1));

		Contract contract = Trans(one_contract);
		CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(one_contract->codeId.c_str());

		//m_pApi->reqMktData(codeHash, contract, "233,236,258", false, false, TagValueListSPtr());
		if (USE_TICK_BY_TICK)
		{
			m_pApi->reqTickByTickData(codeHash, contract, "BidAsk", 0, false);
			//m_pApi->reqTickByTickData(1001, contract, "AllLast", 0, false);
		}
		m_pApi->reqMktData(codeHash, contract, "", false, false, TagValueListSPtr());

		m_pApi->reqMktDepth(codeHash, contract, MaxPriceRowCountSubscribe, false, TagValueListSPtr());

		Log_Print(LogLevel::Warn, fmt::format("SubscribeQuote {} {}", contract.symbol.c_str(), contract.localSymbol.c_str()));
	}

	return;

}

void CIBApi_IB::UnSubscribeQuote(const IbContractPtrs& contracts)
{
	for (auto one_contract : contracts)
	{
		Contract contract = Trans(one_contract);
		CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(one_contract->codeId.c_str());

		m_pApi->cancelMktDepth(codeHash, false);
		m_pApi->cancelMktData(codeHash);

		if (USE_TICK_BY_TICK)
		{
			m_pApi->cancelTickByTickData(codeHash);
		}


	}

	return;

}

bool CIBApi_IB::SendOrder(OneOrderPtr porder)
{
	if (!CIBApi_Real::SendOrder(porder)) return false;

	Order ib_order = TransToIbOrder(porder);
	m_pApi->placeOrder(porder->localOrderNo, Trans(porder->codeHashId), ib_order);
	return true;
}

bool CIBApi_IB::CancelOrder(OneOrderPtr porder, const std::string& manualOrderCancelTime)
{
	if (!CIBApi_Real::CancelOrder(porder, manualOrderCancelTime)) return false;

	m_pApi->cancelOrder(porder->localOrderNo, manualOrderCancelTime);
	return true;
}

Contract CIBApi_IB::Trans(IbContractPtr pcontract)
{
	Contract contract;
	contract.symbol = pcontract->codeId;
	contract.localSymbol = pcontract->localSymbol;
	contract.secType = CTransToStr::Get_SecurityType(pcontract->securityType);
	contract.currency = CTransToStr::Get_Money(pcontract->currencyID);
	contract.exchange = CTransToStr::Get_ExchangePl(pcontract->exchangePl);
	contract.primaryExchange = CTransToStr::Get_ExchangePl(pcontract->primaryExchangePl);
	
	return contract;



}

Contract CIBApi_IB::Trans(CodeHashId codeHash)
{
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeHash);
	if (!contract)
	{
		Log_Print(LogLevel::Err, "can not get contract from codeHash");
		exit(-1);
	}
	return Trans(contract);
}

Order CIBApi_IB::TransToIbOrder(OneOrderPtr porder)
{
	std::string ib_action = "";
	if (porder->buyOrSell == BuyOrSell::Buy)
	{
		ib_action = "BUY";
	}
	else
	{
		ib_action = "SELL";
	}

	// 限价单
	Order order;
	order.action = ib_action;
	order.orderType = "LMT";
	order.totalQuantity = decimal_from_int(CGlobal::DToI(porder->orderQuantity)); ;
	order.lmtPrice = porder->orderPriceReal;
	order.outsideRth = true;	// 允许非常规交易时间进行交易

	return order;

}

std::string CIBApi_IB::TransToBarSize(Time_Type timetype)
{
	std::string back = "";
	switch (timetype)
	{
	case Time_Type::S15:
		back = "15 secs";
		break;
	case Time_Type::M1:
		back = "1 min";
		break;
	case Time_Type::M5:
		back = "5 mins";
		break;
	case Time_Type::M15:
		back = "15 mins";
		break;
	case Time_Type::M30:
		back = "30 mins";
		break;
	case Time_Type::H1:
		back = "1 hour";
		break;
	case Time_Type::D1:
		back = "1 day";
		break;
	default:
		break;
	}
	return back;
}

IBKLinePtrs CIBApi_IB::ChgTimeToRecordeNo(const IBKLinePtrs& klines, Time_Type timeType)
{
	IBKLinePtrs ret;

	for (auto kline : klines)
	{
		kline->time = CGetRecordNo::GetRecordNo(timeType, kline->time);
		ret.push_back(kline);
	}
	return ret;
}

bool CIBApi_IB::TryConnect(time_t millisecond)
{
	OneBrokerPtr broker = MakeAndGet_JSonBroker()->LoadBroker();
	Log_Print(LogLevel::Info, fmt::format("try to connect to {}:{}, connId = {}", broker->gatewayIp.c_str(), broker->gatewayPort, broker->connId));

	time_t beginTime = Get_CurrentTime()->GetCurrentTime_millisecond();
	while (Get_CurrentTime()->GetCurrentTime_millisecond() - beginTime < millisecond)
	{
		bool success = m_pApi->eConnect(broker->gatewayIp.c_str(), broker->gatewayPort, broker->connId);
		if (!success)
		{
			Log_Print(LogLevel::Err, fmt::format("can not connect to TWS or gateway"));
			exit(-1);
		}

		Log_Print(LogLevel::Info, fmt::format("call m_pApi->eConnect success"));

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		if (m_pApi->isConnected()) return true;

		Log_Print(LogLevel::Warn, fmt::format("connection loss, try again"));
	}

	return false;
}

std::string CIBApi_IB::GetIBQueryTimeStr(IbContractPtr contract, time_t timeSecond)
{
	std::string ret = "";

	if (contract->primaryExchangePl == ExchangePl::COMEX)
	{
		ret = CGlobal::GetTimeStr(timeSecond) + " US/Eastern";
		return ret;
	}

	if (contract->securityType == SecurityType::STK)
	{
		ret = CGlobal::GetTimeStr(timeSecond) + " US/Eastern";
	}
	else
	{
		ret = CGlobal::GetTimeStr(timeSecond - 60 * 60) + " US/Central";

	}
	return ret;
}

