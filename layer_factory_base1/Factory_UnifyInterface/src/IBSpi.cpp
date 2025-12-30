#include "pch.h"
#include "IBSpi.h"
#include <Factory_QDatabase.h>
#include <Factory_Topics.h>
#include <Factory_Log.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_Callback.h>
#include <Global.h>
#include <Factory_TShareEnv.h>
#include <Factory_HashEnv.h>

#include <stdio.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <ctime>
#include <fstream>
#include <cstdint>
#include <time.h>
#include <string>

#include <EClientSocket.h>
#include <Utils.h>

// #include <IntelRDFPMathLib.h>


#ifdef WIN32
#include <direct.h>
#endif // WIN32




CIBSpi::CIBSpi()
{
	m_extraAuth = false;
}

void CIBSpi::SetApi(EClientSocket* pApi)
{
	m_pApi = pApi;
}

void CIBSpi::tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib& attrib)
{
	//IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(tickerId);

	std::lock_guard<std::mutex> lock(m_mutex);
	if (field == OPEN)
	{
		MakeAndGet_WhiteBoard()->UpdateOpen(tickerId, price);
	}
	if (field == LAST)
	{
		MakeAndGet_WhiteBoard()->UpdateLastPrice(tickerId, price);
	}

	if (!USE_TICK_BY_TICK)
	{
		if (field == BID)
		{
			//printf("tickPrice rec BID %.2f \n", price);
			MakeAndGet_WhiteBoard()->UpdateBid(tickerId, price);
		}
		if (field == ASK)
		{
			//printf("tickPrice rec ASK %.2f \n", price);
			MakeAndGet_WhiteBoard()->UpdateAsk(tickerId, price);
		}
	}
	return;

}
void CIBSpi::tickSize(TickerId tickerId, TickType field, Decimal size)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(tickerId);
	IBTickPtr tick = nullptr;
	if (field == LAST_SIZE)
	{
		//printf("tickSize rec LAST_SIZE %d \n", size);
		tick = MakeAndGet_WhiteBoard()->UpdateLastVol(tickerId, decimal_to_int(size));

	}
	if (field == VOLUME)
	{
		// tickSize 股票返回的值是手数，需要乘以100转换成股
		//printf("tickSize rec totalvol %d \n", size);
		if (contract->securityType == SecurityType::STK)
		{
			tick = MakeAndGet_WhiteBoard()->UpdateTotalVol(tickerId, decimal_to_int(size) * 100);
		}
		else
		{
			int totalvol = decimal_to_int(size);
			std::string timeStr = CGlobal::GetTickTimeStr(Get_CurrentTime()->GetCurrentTime_millisecond());
			std::string str = fmt::format("{} {} rec totalvol = {}", timeStr.c_str(), contract->codeId.c_str(), totalvol);
			Log_AsyncPrintDailyFile("whiteboard", str, 1, false);

			tick = MakeAndGet_WhiteBoard()->UpdateTotalVol(tickerId, totalvol);
		}

	}

	if (!USE_TICK_BY_TICK)
	{
		if (field == BID_SIZE)
		{
			//printf("tickSize rec bidvol %d \n", size);
			tick = MakeAndGet_WhiteBoard()->UpdateBidVol(tickerId, decimal_to_int(size));
		}
		if (field == ASK_SIZE)
		{
			//printf("tickSize rec askvol %d \n", size);
			tick = MakeAndGet_WhiteBoard()->UpdateAskVol(tickerId, decimal_to_int(size));
		}
	}

	if (!tick) return;

	//Log_Print(LogLevel::Info, fmt::format("rec tick {}, {}({})->{}({}), open = {} last = {} ", 
	//	Get_CodeIdEnv()->Get_CodeStrByHashId(tick->codeHash), 
	//	tick->bidAsks[0].bid,
	//	tick->bidAsks[0].bidVol,
	//	tick->bidAsks[0].ask,
	//	tick->bidAsks[0].askVol,
	//	tick->open,
	//	tick->last
	//));

	IBTickPtr cloneTick = tick->Clone();
	MakeAndGet_MarketDepth()->Combine(tickerId, cloneTick->bidAsks);
	//printf("tick size %d \n", cloneTick->bidAsks.size());
	UpdateTickToApp(tickerId, cloneTick);

	return;
}

void CIBSpi::tickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size, const TickAttribLast& tickAttribLast, const std::string& exchange, const std::string& specialConditions)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	// last由tickPrice更新，不由这里更新
	//if (tickType == 1) return;  // Last更新

	//printf("tick-by-tick rec last = %.2f, lastvol = %d \n", price, size);

	////// AllLast更新
	//IBTickPtr tick = Get_WhiteBoard()->UpdateLast(reqId, price, size);
	//if (!tick) return;

	//MakeAndGet_Topic_OnTick()->UpdateTick_OnTick(tick);
	return;
}


void CIBSpi::tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize, Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	printf("tick-by-tick rec bid = %.2f, bidvol = %d, ask= %.2f, askvol = %d \n",
		bidPrice, decimal_to_int(bidSize), askPrice, decimal_to_int(askSize));
	IBTickPtr  tick = MakeAndGet_WhiteBoard()->UpdateBidAsk(reqId, bidPrice, decimal_to_int(bidSize), askPrice, decimal_to_int(askSize));
	if (!tick) return;

	IBTickPtr cloneTick = tick->Clone();
	MakeAndGet_MarketDepth()->Combine(reqId, cloneTick->bidAsks);
	UpdateTickToApp(reqId, cloneTick);
	return;
}


void CIBSpi::tickByTickMidPoint(int reqId, time_t time, double midPoint)
{
	printf("Tick-By-Tick. ReqId: %d, TickType: MidPoint, Time: %s, MidPoint: %g\n", reqId, ctime(&time), midPoint);
}

void CIBSpi::tickOptionComputation(TickerId tickerId, TickType tickType, int tickAttrib, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice)
{
	printf("TickOptionComputation. Ticker Id: %ld, Type: %d, ImpliedVolatility: %g, Delta: %g, OptionPrice: %g, pvDividend: %g, Gamma: %g, Vega: %g, Theta: %g, Underlying Price: %g\n", tickerId, (int)tickType, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice);
}

void CIBSpi::tickGeneric(TickerId tickerId, TickType tickType, double value)
{
	//printf("Tick Generic. Ticker Id: %ld, Type: %d, Value: %g\n", tickerId, (int)tickType, value);
}

void CIBSpi::tickString(TickerId tickerId, TickType tickType, const std::string& value)
{
	// printf("Tick String. Ticker Id: %ld, Type: %d, Value: %s\n", tickerId, (int)tickType, value.c_str());
}

void CIBSpi::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints, double totalDividends, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate)
{
	printf("TickEFP. %ld, Type: %d, BasisPoints: %g, FormattedBasisPoints: %s, Total Dividends: %g, HoldDays: %d, Future Last Trade Date: %s, Dividend Impact: %g, Dividends To Last Trade Date: %g\n", tickerId, (int)tickType, basisPoints, formattedBasisPoints.c_str(), totalDividends, holdDays, futureLastTradeDate.c_str(), dividendImpact, dividendsToLastTradeDate);
}

void CIBSpi::orderStatus(OrderId orderId, const std::string& status, Decimal filled, Decimal remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice)
{
	//printf("OrderStatus. Id: %ld, Status: %s, Filled: %s, Remaining: %s, AvgFillPrice: %s, PermId: %s, LastFillPrice: %s, ClientId: %s, WhyHeld: %s, MktCapPrice: %s\n",
	//	orderId, status.c_str(), decimalStringToDisplay(filled).c_str(), decimalStringToDisplay(remaining).c_str(), Utils::doubleMaxString(avgFillPrice).c_str(), Utils::intMaxString(permId).c_str(),
	//	Utils::doubleMaxString(lastFillPrice).c_str(), Utils::intMaxString(clientId).c_str(), whyHeld.c_str(), Utils::doubleMaxString(mktCapPrice).c_str());

	std::string avgFillPriceStr = Utils::doubleMaxString(avgFillPrice);
	std::string lastFillPriceStr = Utils::doubleMaxString(lastFillPrice);
	// printf("AvgFillPrice: %s, LastFillPrice: %s \n", avgFillPriceStr.c_str(), lastFillPriceStr.c_str());

	if (avgFillPriceStr != "" && lastFillPriceStr != "")
	{
		std::string temstr = fmt::format("ib api receive orderstatus : avgFillPrice = {}, lastFillPrice = {}", avgFillPriceStr.c_str(), lastFillPriceStr.c_str());
		int modSecond = 1;
		Log_AsyncPrintRotatingFile("orderprice.log", temstr, modSecond, false);
	}


	OrderStatus orderStatus;
	if (!TransOrderStatus(status, orderStatus)) return;

	OneOrderPtr pOriginalOrder = MakeAndGet_DbActiveOrder()->GetOne(orderId);
	if (!pOriginalOrder)
	{
		//Log_Print(LogLevel::Warn, fmt::format("CIBSpi::orderStatus, order {} is not in active orders \n", orderId));
		return;
	}
	if (pOriginalOrder->orderStatus == OrderStatus::AllDealed)
	{
		// 数据库中已经是全部成交状态，这是重复的发送
		return;
	}

	// 委托已成交量
	double totalDealToday = decimal_To_double(filled);
	// 委托已成交的平均价格
	double todayDealAvgPrice = avgFillPrice;

	// 当下这笔新的成交量
	double thisDealVol = totalDealToday - pOriginalOrder->dealedQuantity;
	// 当下这笔新的成交价格 
	double thisDealPrice = lastFillPrice;

	// 制作委托回报
	OnOrderMsg onOrderMsg;
	onOrderMsg.orderStatus = orderStatus;
	onOrderMsg.todayDealedAvgPrice = avgFillPrice;
	onOrderMsg.todayDealedVol = totalDealToday;
	onOrderMsg.pOriginalOrder = pOriginalOrder;
	PushOnOrderMsgs(onOrderMsg);




	// 制作成交回报
	if (CGlobal::DToI(thisDealVol) != 0)
	{
		OnTradeMsg onTradeMsg;
		onTradeMsg.dealNo = int(MakeAndGet_SeriesNoMaker()->GetSeriesNo());
		onTradeMsg.dealPrice = thisDealPrice;
		onTradeMsg.dealQuantity = thisDealVol;
		onTradeMsg.pOriginalOrder = pOriginalOrder;
		PushOnTradeMsgs(onTradeMsg);
	}

	return;


}

void CIBSpi::openOrder(OrderId orderId, const Contract& contract, const Order& order, const OrderState& orderState)
{
	// 回调的具体顺序参见 EWrapper IB回调函数说明.txt 仅处理OrderStatus回调即可
	// 委托回报
	//printf("OpenOrder. PermId: %s, ClientId: %s, OrderId: %s, Account: %s, Symbol: %s, SecType: %s, Exchange: %s:, Action: %s, OrderType:%s, TotalQty: %s, CashQty: %s, "
	//	"LmtPrice: %s, AuxPrice: %s, Status: %s, MinTradeQty: %s, MinCompeteSize: %s, CompeteAgainstBestOffset: %s, MidOffsetAtWhole: %s, MidOffsetAtHalf: %s\n",
	//	Utils::intMaxString(order.permId).c_str(), Utils::longMaxString(order.clientId).c_str(), Utils::longMaxString(orderId).c_str(), order.account.c_str(), contract.symbol.c_str(),
	//	contract.secType.c_str(), contract.exchange.c_str(), order.action.c_str(), order.orderType.c_str(), decimalStringToDisplay(order.totalQuantity).c_str(),
	//	Utils::doubleMaxString(order.cashQty).c_str(), Utils::doubleMaxString(order.lmtPrice).c_str(), Utils::doubleMaxString(order.auxPrice).c_str(), orderState.status.c_str(),
	//	Utils::intMaxString(order.minTradeQty).c_str(), Utils::intMaxString(order.minCompeteSize).c_str(),
	//	order.competeAgainstBestOffset == COMPETE_AGAINST_BEST_OFFSET_UP_TO_MID ? "UpToMid" : Utils::doubleMaxString(order.competeAgainstBestOffset).c_str(),
	//	Utils::doubleMaxString(order.midOffsetAtWhole).c_str(), Utils::doubleMaxString(order.midOffsetAtHalf).c_str());

	//OrderStatus orderStatus;
	//if (!TransOrderStatus(orderState.status, orderStatus)) return;

	//OneOrderPtr pOriginalOrder = MakeAndGet_DbActiveOrder()->GetOne(orderId);
	//if (!pOriginalOrder)
	//{
	//	//Log_Print(LogLevel::Warn, fmt::format("CIBSpi::openOrder, order {} is not in active orders \n", orderId));
	//	return;
	//}
	//double totalDealToday = 0.0;
	//if (orderStatus == OrderStatus::AllDealed)
	//{
	//	totalDealToday = pOriginalOrder->orderQuantity;
	//}
	//OnRtnPtr pOnRtnOrder = nullptr;
	//if (CMixCodeHelper::IsMix(pOriginalOrder->orderKey.strategyIdHashId))
	//{
	//	pOnRtnOrder = Make_OnRtn_Order_MixCode(orderStatus, totalDealToday, pOriginalOrder);
	//}
	//else
	//{
	//	pOnRtnOrder = Make_OnRtn_Order(orderStatus, totalDealToday, pOriginalOrder);
	//}
	//pOnRtnOrder->Execute();
	return;
}

void CIBSpi::openOrderEnd()
{
	printf("OpenOrderEnd\n");
}

void CIBSpi::winError(const std::string& str, int lastError)
{

}

void CIBSpi::connectionClosed()
{
	printf("Connection Closed\n");
}

void CIBSpi::updateAccountValue(const std::string& key, const std::string& val, const std::string& currency, const std::string& accountName)
{
	printf("UpdateAccountValue. Key: %s, Value: %s, Currency: %s, Account Name: %s\n", key.c_str(), val.c_str(), currency.c_str(), accountName.c_str());
}

void CIBSpi::updatePortfolio(const Contract& contract, Decimal position, double marketPrice, double marketValue, double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName)
{
	printf("UpdatePortfolio. %s, %s @ %s: Position: %g, MarketPrice: %g, MarketValue: %g, AverageCost: %g, UnrealizedPNL: %g, RealizedPNL: %g, AccountName: %s\n", (contract.symbol).c_str(), (contract.secType).c_str(), (contract.primaryExchange).c_str(), position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountName.c_str());
}

void CIBSpi::updateAccountTime(const std::string& timeStamp)
{
	printf("UpdateAccountTime. Time: %s\n", timeStamp.c_str());
}

void CIBSpi::accountDownloadEnd(const std::string& accountName)
{
	printf("Account download finished: %s\n", accountName.c_str());
}

void CIBSpi::nextValidId(OrderId orderId)
{
	MakeAndGet_SeriesNoMaker()->SetSeriesNo(orderId);
	Log_Print(LogLevel::Info, fmt::format("Next Valid Id: {}", orderId));
}

void CIBSpi::contractDetails(int reqId, const ContractDetails& contractDetails)
{
	printf("ContractDetails begin. ReqId: %d\n", reqId);
	printContractMsg(contractDetails.contract);
	printContractDetailsMsg(contractDetails);
	printf("ContractDetails end. ReqId: %d\n", reqId);

}

void CIBSpi::bondContractDetails(int reqId, const ContractDetails& contractDetails)
{
	printf("BondContractDetails begin. ReqId: %d\n", reqId);
	printBondContractDetailsMsg(contractDetails);
	printf("BondContractDetails end. ReqId: %d\n", reqId);

}

void CIBSpi::contractDetailsEnd(int reqId)
{
	printf("ContractDetailsEnd. %d\n", reqId);
}

void CIBSpi::execDetails(int reqId, const Contract& contract, const Execution& execution)
{
	// 回调的具体顺序参见 EWrapper IB回调函数说明.txt 仅处理OrderStatus回调即可
	//printf("ExecDetails. ReqId: %d - %s, %s, %s - %s, %s, %s, %s, %s\n", reqId, contract.symbol.c_str(), contract.secType.c_str(), contract.currency.c_str(), execution.execId.c_str(), Utils::longMaxString(execution.orderId).c_str(), decimalStringToDisplay(execution.shares).c_str(), decimalStringToDisplay(execution.cumQty).c_str(), Utils::intMaxString(execution.lastLiquidity).c_str());

	//OneOrderPtr pOriginalOrder = MakeAndGet_DbActiveOrder()->GetOne(execution.orderId);
	//if (!pOriginalOrder)
	//{
	//	//Log_Print(LogLevel::Warn, fmt::format("CIBSpi::execDetails, order {} is not in active orders \n", execution.orderId));
	//	return;
	//}

	//double totalDealToday = decimal_To_double(execution.shares);

	//OnRtnPtr pOnRtnOrder = nullptr;
	//if (CMixCodeHelper::IsMix(pOriginalOrder->orderKey.strategyIdHashId))
	//{
	//	pOnRtnOrder = Make_OnRtn_Order_MixCode(OrderStatus::SomeDealed, totalDealToday, pOriginalOrder);
	//}
	//else
	//{
	//	pOnRtnOrder = Make_OnRtn_Order(OrderStatus::SomeDealed, totalDealToday, pOriginalOrder);

	//}
	//pOnRtnOrder->Execute();
	return;

}

void CIBSpi::execDetailsEnd(int reqId)
{
	printf("ExecDetailsEnd. %d\n", reqId);
}

void CIBSpi::error(int id, int errorCode, const std::string& errorString, const std::string& advancedOrderRejectJson)
{
	if (errorCode == 202)
	{
		// 正常撤单
		return;
	}
	Log_Print(LogLevel::Err, fmt::format("Error. Id: {}, Code: {}, Msg1: {}, Msg2: {}",
		id,
		errorCode,
		CGlobal::decode_unicode(errorString).c_str(),
		CGlobal::decode_unicode(advancedOrderRejectJson).c_str()
	));
	//printf("Error. Id: %d, Code: %d, Msg: %s\n", id, errorCode, errorString.c_str());

	if (errorCode == 10148)
	{
		// 撤单失败
		OneOrderPtr originalOrder = GetOrderFromActiveOrInactive(id);
		if (!originalOrder) return;

		OnRtnPtr pOnRtn = Make_OnRtn_ErrCancel(originalOrder);
		pOnRtn->Execute();
		return;
	}
	if (errorCode == 162)
	{
		// 历史kline数据已经查询完毕
		MakeAndGet_KLineBoard()->SetKLineBoardStatus(KLineBoardStatus::Done);
	}
	if (errorCode == 10187)
	{
		// 历史tick数据已经查询完毕
		MakeAndGet_TickBoard()->SetTickBoardStatus(TickBoardStatus::Done);
	}

}

void CIBSpi::updateMktDepth(TickerId id, int position, int operation, int side, double price, Decimal size)
{
	//printf("UpdateMarketDepth. %ld - Position: %d, Operation: %d, Side: %d, Price: %g, Size: %d\n", id, position, operation, side, price, size);
	//IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(id);
	//printf("%s call CIBSpi::updateMktDepth %d \n", contract->codeId.c_str(), position);

	if (side == 0)
	{
		// ASK
		if (operation == 0)
		{
			MakeAndGet_MarketDepth()->Add(id, BidOrAsk::Ask, position, price, decimal_to_int(size));
		}
		if (operation == 1)
		{
			MakeAndGet_MarketDepth()->Update(id, BidOrAsk::Ask, position, price, decimal_to_int(size));
		}
		if (operation == 2)
		{
			MakeAndGet_MarketDepth()->Delete(id, BidOrAsk::Ask, position);
		}

	}
	else
	{
		// BID
		if (operation == 0)
		{
			MakeAndGet_MarketDepth()->Add(id, BidOrAsk::Bid, position, price, decimal_to_int(size));
		}
		if (operation == 1)
		{
			MakeAndGet_MarketDepth()->Update(id, BidOrAsk::Bid, position, price, decimal_to_int(size));
		}
		if (operation == 2)
		{
			MakeAndGet_MarketDepth()->Delete(id, BidOrAsk::Bid, position);
		}
	}

}

void CIBSpi::updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation, int side, double price, Decimal size, bool isSmartDepth)
{
	printf("UpdateMarketDepthL2. %ld - Position: %d, Operation: %d, Side: %d, Price: %g, Size: %d, isSmartDepth: %d\n", id, position, operation, side, price, size, isSmartDepth);
}

void CIBSpi::updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, const std::string& originExch)
{
	printf("News Bulletins. %d - Type: %d, Message: %s, Exchange of Origin: %s\n", msgId, msgType, newsMessage.c_str(), originExch.c_str());
}

void CIBSpi::managedAccounts(const std::string& accountsList)
{
	printf("Account List: %s\n", accountsList.c_str());
}

void CIBSpi::receiveFA(faDataType pFaDataType, const std::string& cxml)
{
	std::cout << "Receiving FA: " << (int)pFaDataType << std::endl << cxml << std::endl;
}

void CIBSpi::historicalData(TickerId codeId, const Bar& bar)
{
	//printf("HistoricalData. ReqId: %ld - Date: %s, Open: %s, High: %s, Low: %s, Close: %s, Volume: %s, Count: %s, WAP: %s\n", codeId, bar.time.c_str(),
	//	Utils::doubleMaxString(bar.open).c_str(), Utils::doubleMaxString(bar.high).c_str(), Utils::doubleMaxString(bar.low).c_str(), Utils::doubleMaxString(bar.close).c_str(),
	//	decimalStringToDisplay(bar.volume).c_str(), Utils::intMaxString(bar.count).c_str(), decimalStringToDisplay(bar.wap).c_str());

	std::string barTimeStr = "";
	std::string timeZoneStr = "";

	TransBarTime(bar.time, barTimeStr, timeZoneStr);
	if (barTimeStr.length() == 8)
	{
		// 日线
		barTimeStr += " 00:00:00";
	}

	IBKLinePtr kline = std::make_shared<CIBKLine>();
	kline->time = GetTimeByIBStr(barTimeStr, timeZoneStr) * 1000;
	kline->open = bar.open;
	kline->close = bar.close;
	kline->high = bar.high;
	kline->low = bar.low;
	kline->vol = decimal_to_int(bar.volume);

	MakeAndGet_KLineBoard()->AddOne(kline);

	//Log_Print(LogLevel::Info, fmt::format("{} open = {:.2f}, close = {:.2f}, high = {:.2f}, low = {:.2f}, vol = {}",
	//	CGlobal::GetTickTimeStr(kline->time),
	//	kline->open, kline->close, kline->high, kline->low, kline->vol
	//	) );

	//Log_Print(LogLevel::Info, fmt::format("receive {} {}, result = {}", barTimeStr.c_str(), timeZoneStr.c_str(), CGlobal::GetTickTimeStr(kline->time).c_str() ));



	return;


}

void CIBSpi::historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr)
{
	//std::cout << "HistoricalDataEnd. ReqId: " << reqId << " - Start Date: " << startDateStr << ", End Date: " << endDateStr << std::endl;
	MakeAndGet_KLineBoard()->SetKLineBoardStatus(KLineBoardStatus::Done);
}

void CIBSpi::scannerParameters(const std::string& xml)
{
	printf("ScannerParameters. %s\n", xml.c_str());
}

void CIBSpi::scannerData(int reqId, int rank, const ContractDetails& contractDetails, const std::string& distance, const std::string& benchmark, const std::string& projection, const std::string& legsStr)
{
	printf("ScannerData. %d - Rank: %d, Symbol: %s, SecType: %s, Currency: %s, Distance: %s, Benchmark: %s, Projection: %s, Legs String: %s\n", reqId, rank, contractDetails.contract.symbol.c_str(), contractDetails.contract.secType.c_str(), contractDetails.contract.currency.c_str(), distance.c_str(), benchmark.c_str(), projection.c_str(), legsStr.c_str());
}

void CIBSpi::scannerDataEnd(int reqId)
{
	printf("ScannerDataEnd. %d\n", reqId);
}

void CIBSpi::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close, Decimal volume, Decimal wap, int count)
{
	printf("RealTimeBars. %ld - Time: %ld, Open: %g, High: %g, Low: %g, Close: %g, Volume: %ld, Count: %d, WAP: %g\n", reqId, time, open, high, low, close, volume, count, wap);
}

void CIBSpi::currentTime(long time)
{

}

void CIBSpi::fundamentalData(TickerId reqId, const std::string& data)
{
	printf("FundamentalData. ReqId: %ld, %s\n", reqId, data.c_str());
}

void CIBSpi::deltaNeutralValidation(int reqId, const DeltaNeutralContract& deltaNeutralContract)
{
	printf("DeltaNeutralValidation. %d, ConId: %ld, Delta: %g, Price: %g\n", reqId, deltaNeutralContract.conId, deltaNeutralContract.delta, deltaNeutralContract.price);
}

void CIBSpi::tickSnapshotEnd(int reqId)
{
	printf("TickSnapshotEnd: %d\n", reqId);
}

void CIBSpi::marketDataType(TickerId reqId, int marketDataType)
{
	printf("MarketDataType. ReqId: %ld, Type: %d\n", reqId, marketDataType);
}

void CIBSpi::commissionReport(const CommissionReport& commissionReport)
{
	//printf("CommissionReport. %s - %g %s RPNL %g\n", commissionReport.execId.c_str(), commissionReport.commission, commissionReport.currency.c_str(), commissionReport.realizedPNL);
}

void CIBSpi::position(const std::string& account, const Contract& contract, Decimal position, double avgCost)
{
	printf("Position. %s - Symbol: %s, SecType: %s, Currency: %s, Position: %g, Avg Cost: %g\n", account.c_str(), contract.symbol.c_str(), contract.secType.c_str(), contract.currency.c_str(), position, avgCost);
}

void CIBSpi::positionEnd()
{
	printf("PositionEnd\n");
}

void CIBSpi::accountSummary(int reqId, const std::string& account, const std::string& tag, const std::string& value, const std::string& curency)
{
	printf("Acct Summary. ReqId: %d, Account: %s, Tag: %s, Value: %s, Currency: %s\n", reqId, account.c_str(), tag.c_str(), value.c_str(), curency.c_str());
}

void CIBSpi::accountSummaryEnd(int reqId)
{
	printf("AccountSummaryEnd. Req Id: %d\n", reqId);
}

void CIBSpi::verifyMessageAPI(const std::string& apiData)
{
	printf("verifyMessageAPI: %s\b", apiData.c_str());
}

void CIBSpi::verifyCompleted(bool isSuccessful, const std::string& errorText)
{
	printf("verifyCompleted. IsSuccessfule: %d - Error: %s\n", isSuccessful, errorText.c_str());
}

void CIBSpi::displayGroupList(int reqId, const std::string& groups)
{
	printf("Display Group List. ReqId: %d, Groups: %s\n", reqId, groups.c_str());
}

void CIBSpi::displayGroupUpdated(int reqId, const std::string& contractInfo)
{
	std::cout << "Display Group Updated. ReqId: " << reqId << ", Contract Info: " << contractInfo << std::endl;
}

void CIBSpi::verifyAndAuthMessageAPI(const std::string& apiData, const std::string& xyzChallange)
{
	printf("verifyAndAuthMessageAPI: %s %s\n", apiData.c_str(), xyzChallange.c_str());
}

void CIBSpi::verifyAndAuthCompleted(bool isSuccessful, const std::string& errorText)
{
	printf("verifyAndAuthCompleted. IsSuccessful: %d - Error: %s\n", isSuccessful, errorText.c_str());
	if (isSuccessful)
		m_pApi->startApi();

}

void CIBSpi::connectAck()
{
	if (!m_extraAuth && m_pApi->asyncEConnect())
		m_pApi->startApi();

}

void CIBSpi::positionMulti(int reqId, const std::string& account, const std::string& modelCode, const Contract& contract, Decimal pos, double avgCost)
{
	printf("Position Multi. Request: %d, Account: %s, ModelCode: %s, Symbol: %s, SecType: %s, Currency: %s, Position: %g, Avg Cost: %g\n", reqId, account.c_str(), modelCode.c_str(), contract.symbol.c_str(), contract.secType.c_str(), contract.currency.c_str(), pos, avgCost);
}


void CIBSpi::positionMultiEnd(int reqId)
{
	printf("Position Multi End. Request: %d\n", reqId);
}


void CIBSpi::accountUpdateMulti(int reqId, const std::string& account, const std::string& modelCode, const std::string& key, const std::string& value, const std::string& currency)
{
	printf("AccountUpdate Multi. Request: %d, Account: %s, ModelCode: %s, Key, %s, Value: %s, Currency: %s\n", reqId, account.c_str(), modelCode.c_str(), key.c_str(), value.c_str(), currency.c_str());
}


void CIBSpi::accountUpdateMultiEnd(int reqId)
{
	printf("Account Update Multi End. Request: %d\n", reqId);
}


void CIBSpi::securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass,
	const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes)
{
	printf("Security Definition Optional Parameter. Request: %d, Trading Class: %s, Multiplier: %s\n", reqId, tradingClass.c_str(), multiplier.c_str());
}


void CIBSpi::securityDefinitionOptionalParameterEnd(int reqId)
{
	printf("Security Definition Optional Parameter End. Request: %d\n", reqId);
}


void CIBSpi::softDollarTiers(int reqId, const std::vector<SoftDollarTier>& tiers)
{
	printf("Soft dollar tiers (%lu):", tiers.size());

	for (unsigned int i = 0; i < tiers.size(); i++) {
		printf("%s\n", tiers[i].displayName().c_str());
	}

}


void CIBSpi::familyCodes(const std::vector<FamilyCode>& familyCodes)
{
	printf("Family codes (%lu):\n", familyCodes.size());

	for (unsigned int i = 0; i < familyCodes.size(); i++) {
		printf("Family code [%d] - accountID: %s familyCodeStr: %s\n", i, familyCodes[i].accountID.c_str(), familyCodes[i].familyCodeStr.c_str());
	}

}


void CIBSpi::symbolSamples(int reqId, const std::vector<ContractDescription>& contractDescriptions)
{
	printf("Symbol Samples (total=%lu) reqId: %d\n", contractDescriptions.size(), reqId);

	for (unsigned int i = 0; i < contractDescriptions.size(); i++) {
		Contract contract = contractDescriptions[i].contract;
		std::vector<std::string> derivativeSecTypes = contractDescriptions[i].derivativeSecTypes;
		printf("Contract (%u): conId: %ld, symbol: %s, secType: %s, primaryExchange: %s, currency: %s, ", i, contract.conId, contract.symbol.c_str(), contract.secType.c_str(), contract.primaryExchange.c_str(), contract.currency.c_str());
		printf("Derivative Sec-types (%lu):", derivativeSecTypes.size());
		for (unsigned int j = 0; j < derivativeSecTypes.size(); j++) {
			printf(" %s", derivativeSecTypes[j].c_str());
		}
		printf(", description: %s, issuerId: %s", contract.description.c_str(), contract.issuerId.c_str());
		printf("\n");
	}

}


void CIBSpi::mktDepthExchanges(const std::vector<DepthMktDataDescription>& depthMktDataDescriptions)
{
	printf("Mkt Depth Exchanges (%llu):\n", depthMktDataDescriptions.size());

	for (unsigned int i = 0; i < depthMktDataDescriptions.size(); i++) {
		printf("Depth Mkt Data Description [%d] - exchange: %s secType: %s listingExch: %s serviceDataType: %s aggGroup: %s\n", i,
			depthMktDataDescriptions[i].exchange.c_str(),
			depthMktDataDescriptions[i].secType.c_str(),
			depthMktDataDescriptions[i].listingExch.c_str(),
			depthMktDataDescriptions[i].serviceDataType.c_str(),
			depthMktDataDescriptions[i].aggGroup != INT_MAX ? std::to_string(depthMktDataDescriptions[i].aggGroup).c_str() : "");
	}

}


void CIBSpi::tickNews(int tickerId, time_t timeStamp, const std::string& providerCode, const std::string& articleId, const std::string& headline, const std::string& extraData)
{
	printf("News Tick. TickerId: %d, TimeStamp: %s, ProviderCode: %s, ArticleId: %s, Headline: %s, ExtraData: %s\n", tickerId, ctime(&(timeStamp /= 1000)), providerCode.c_str(), articleId.c_str(), headline.c_str(), extraData.c_str());
}

void CIBSpi::smartComponents(int reqId, const SmartComponentsMap& theMap)
{
	printf("Smart components: (%llu):\n", theMap.size());

	for (SmartComponentsMap::const_iterator i = theMap.begin(); i != theMap.end(); i++) {
		printf(" bit number: %d exchange: %s exchange letter: %c\n", i->first, std::get<0>(i->second).c_str(), std::get<1>(i->second));
	}

}


void CIBSpi::tickReqParams(int tickerId, double minTick, const std::string& bboExchange, int snapshotPermissions)
{
	printf("tickerId: %d, minTick: %g, bboExchange: %s, snapshotPermissions: %u \n", tickerId, minTick, bboExchange.c_str(), snapshotPermissions);
}


void CIBSpi::newsProviders(const std::vector<NewsProvider>& newsProviders)
{
	printf("News providers (%lu):\n", newsProviders.size());

	for (unsigned int i = 0; i < newsProviders.size(); i++) {
		printf("News provider [%d] - providerCode: %s providerName: %s\n", i, newsProviders[i].providerCode.c_str(), newsProviders[i].providerName.c_str());
	}

}


void CIBSpi::newsArticle(int requestId, int articleType, const std::string& articleText)
{
	printf("News Article. Request Id: %d, Article Type: %d\n", requestId, articleType);
	if (articleType == 0) {
		printf("News Article Text (text or html): %s\n", articleText.c_str());
	}
	else if (articleType == 1) {
		std::string path;
#if defined(IB_WIN32)
		//TCHAR s[200];
		//GetCurrentDirectory(200, s);
		char s[200];
		_getcwd(s, 200);

		path = std::string(s) + std::string("\\MST$06f53098.pdf");
#elif defined(IB_POSIX)
		char s[1024];
		if (getcwd(s, sizeof(s)) == NULL) {
			printf("getcwd() error\n");
			return;
		}
		path = s + std::string("/MST$06f53098.pdf");
#endif
		std::vector<std::uint8_t> bytes = Utils::base64_decode(articleText);
		std::ofstream outfile(path, std::ios::out | std::ios::binary);
		outfile.write((const char*)bytes.data(), bytes.size());
		printf("Binary/pdf article was saved to: %s\n", path.c_str());
	}

}


void CIBSpi::historicalNews(int requestId, const std::string& time, const std::string& providerCode, const std::string& articleId, const std::string& headline)
{
	printf("Historical News. RequestId: %d, Time: %s, ProviderCode: %s, ArticleId: %s, Headline: %s\n", requestId, time.c_str(), providerCode.c_str(), articleId.c_str(), headline.c_str());
}

void CIBSpi::historicalNewsEnd(int requestId, bool hasMore)
{
	printf("Historical News End. RequestId: %d, HasMore: %s\n", requestId, (hasMore ? "true" : " false"));
}


void CIBSpi::headTimestamp(int reqId, const std::string& headTimestamp)
{
	printf("Head time stamp. ReqId: %d - Head time stamp: %s,\n", reqId, headTimestamp.c_str());
}


void CIBSpi::histogramData(int reqId, const HistogramDataVector& data)
{
	printf("Histogram. ReqId: %d, data length: %llu\n", reqId, data.size());

	for (auto item : data) {
		printf("\t price: %f, size: %lld\n", item.price, item.size);
	}

}


void CIBSpi::historicalDataUpdate(TickerId reqId, const Bar& bar)
{
	printf("HistoricalDataUpdate. ReqId: %ld - Date: %s, Open: %g, High: %g, Low: %g, Close: %g, Volume: %lld, Count: %d, WAP: %g\n", reqId, bar.time.c_str(), bar.open, bar.high, bar.low, bar.close, bar.volume, bar.count, bar.wap);
}


void CIBSpi::rerouteMktDataReq(int reqId, int conid, const std::string& exchange)
{
	printf("Re-route market data request. ReqId: %d, ConId: %d, Exchange: %s\n", reqId, conid, exchange.c_str());
}


void CIBSpi::rerouteMktDepthReq(int reqId, int conid, const std::string& exchange)
{
	printf("Re-route market depth request. ReqId: %d, ConId: %d, Exchange: %s\n", reqId, conid, exchange.c_str());
}


void CIBSpi::marketRule(int marketRuleId, const std::vector<PriceIncrement>& priceIncrements)
{
	printf("Market Rule Id: %d\n", marketRuleId);
	for (unsigned int i = 0; i < priceIncrements.size(); i++) {
		printf("Low Edge: %g, Increment: %g\n", priceIncrements[i].lowEdge, priceIncrements[i].increment);
	}

}


void CIBSpi::pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL)
{
	printf("PnL. ReqId: %d, daily PnL: %g, unrealized PnL: %g, realized PnL: %g\n", reqId, dailyPnL, unrealizedPnL, realizedPnL);
}


void CIBSpi::pnlSingle(int reqId, Decimal pos, double dailyPnL, double unrealizedPnL, double realizedPnL, double value)
{
	printf("PnL Single. ReqId: %d, pos: %d, daily PnL: %g, unrealized PnL: %g, realized PnL: %g, value: %g\n", reqId, pos, dailyPnL, unrealizedPnL, realizedPnL, value);
}


void CIBSpi::historicalTicks(int reqId, const std::vector<HistoricalTick>& ticks, bool done)
{
	for (const HistoricalTick& tick : ticks)
	{
		std::time_t t = tick.time;
		std::cout << "Historical tick. ReqId: " << reqId << ", time: " << ctime(&t) << ", price: " << Utils::doubleMaxString(tick.price).c_str() << ", size: " << decimalStringToDisplay(tick.size).c_str() << std::endl;
	}

}


void CIBSpi::historicalTicksBidAsk(int reqId, const std::vector<HistoricalTickBidAsk>& ticks, bool done)
{
	for (const HistoricalTickBidAsk& tick : ticks) {
		std::time_t t = tick.time;
		std::cout << "Historical tick bid/ask. ReqId: " << reqId << ", time: " << ctime(&t) << ", price bid: " << tick.priceBid <<
			", price ask: " << tick.priceAsk << ", size bid: " << tick.sizeBid << ", size ask: " << tick.sizeAsk <<
			", bidPastLow: " << tick.tickAttribBidAsk.bidPastLow << ", askPastHigh: " << tick.tickAttribBidAsk.askPastHigh << std::endl;
	}

}


void CIBSpi::historicalTicksLast(int reqId, const std::vector<HistoricalTickLast>& ticks, bool done)
{
	//for (HistoricalTickLast tick : ticks) {
	//	std::time_t t = tick.time;
	//	std::cout << "Historical tick last. ReqId: " << reqId << ", time: " << ctime(&t) << ", price: " << Utils::doubleMaxString(tick.price).c_str() <<
	//		", size: " << decimalStringToDisplay(tick.size).c_str() << ", exchange: " << tick.exchange << ", special conditions: " << tick.specialConditions <<
	//		", unreported: " << tick.tickAttribLast.unreported << ", pastLimit: " << tick.tickAttribLast.pastLimit << std::endl;
	//}

	if (!done)
	{
		Log_Print(LogLevel::Warn, "in historicalTicksLast, done = false");
		return;
	}

	CodeHashId  codeIdHash = MakeAndGet_TickBoard()->GetCodeHash();
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeIdHash);
	time_t currentSecond = 0;
	time_t currentSecondCount = 0;
	for (auto onetick : ticks)
	{
		IBTickPtr tick = std::make_shared<CIBTick>();
		tick->codeHash = codeIdHash;

		// onetick.time只精确到秒，而且同一秒内有多个tick
		if (currentSecond != onetick.time)
		{
			currentSecond = onetick.time;
			currentSecondCount = 0;
		}
		else
		{
			currentSecondCount++;
		}
		tick->time = currentSecond * 1000 + currentSecondCount;
		tick->last = CGlobal::DToI(onetick.price / contract->minMove);
		tick->vol = decimal_to_int(onetick.size) * 100;
		tick->timeStamp = tick->time * 1000 * 1000;

		MakeAndGet_TickBoard()->AddOne(tick);
	}
	MakeAndGet_TickBoard()->SetTickBoardStatus(TickBoardStatus::Done);

	return;


}




void CIBSpi::orderBound(long long orderId, int apiClientId, int apiOrderId)
{
	printf("Order bound. OrderId: %lld, ApiClientId: %d, ApiOrderId: %d\n", orderId, apiClientId, apiOrderId);
}


void CIBSpi::completedOrder(const Contract& contract, const Order& order, const OrderState& orderState)
{
	printf("CompletedOrder. PermId: %ld, ParentPermId: %lld, Account: %s, Symbol: %s, SecType: %s, Exchange: %s:, Action: %s, OrderType: %s, TotalQty: %g, CashQty: %g, FilledQty: %g, "
		"LmtPrice: %g, AuxPrice: %g, Status: %s, CompletedTime: %s, CompletedStatus: %s\n",
		order.permId, order.parentPermId == UNSET_LONG ? 0 : order.parentPermId, order.account.c_str(), contract.symbol.c_str(), contract.secType.c_str(), contract.exchange.c_str(),
		order.action.c_str(), order.orderType.c_str(), order.totalQuantity, order.cashQty == UNSET_DOUBLE ? 0 : order.cashQty, order.filledQuantity,
		order.lmtPrice, order.auxPrice, orderState.status.c_str(), orderState.completedTime.c_str(), orderState.completedStatus.c_str());

}


void CIBSpi::completedOrdersEnd()
{
	printf("CompletedOrdersEnd\n");
}

void CIBSpi::replaceFAEnd(int reqId, const std::string& text)
{
	printf("replaceFAEnd new\n");
}

void CIBSpi::wshMetaData(int reqId, const std::string& dataJson)
{
	printf("wshMetaData new\n");
}

void CIBSpi::wshEventData(int reqId, const std::string& dataJson)
{
	printf("wshEventData new\n");
}

void CIBSpi::historicalSchedule(int reqId, const std::string& startDateTime, const std::string& endDateTime, const std::string& timeZone, const std::vector<HistoricalSession>& sessions)
{
	printf("historicalSchedule new\n");
}

void CIBSpi::userInfo(int reqId, const std::string& whiteBrandingId)
{
	printf("userInfo new\n");
}

void CIBSpi::printContractMsg(const Contract& contract)
{
	printf("\tConId: %ld\n", contract.conId);
	printf("\tSymbol: %s\n", contract.symbol.c_str());
	printf("\tSecType: %s\n", contract.secType.c_str());
	printf("\tLastTradeDateOrContractMonth: %s\n", contract.lastTradeDateOrContractMonth.c_str());
	printf("\tStrike: %g\n", contract.strike);
	printf("\tRight: %s\n", contract.right.c_str());
	printf("\tMultiplier: %s\n", contract.multiplier.c_str());
	printf("\tExchange: %s\n", contract.exchange.c_str());
	printf("\tPrimaryExchange: %s\n", contract.primaryExchange.c_str());
	printf("\tCurrency: %s\n", contract.currency.c_str());
	printf("\tLocalSymbol: %s\n", contract.localSymbol.c_str());
	printf("\tTradingClass: %s\n", contract.tradingClass.c_str());

}

void CIBSpi::printContractDetailsMsg(const ContractDetails& contractDetails)
{
	printf("\tMarketName: %s\n", contractDetails.marketName.c_str());
	printf("\tMinTick: %g\n", contractDetails.minTick);
	printf("\tPriceMagnifier: %ld\n", contractDetails.priceMagnifier);
	printf("\tOrderTypes: %s\n", contractDetails.orderTypes.c_str());
	printf("\tValidExchanges: %s\n", contractDetails.validExchanges.c_str());
	printf("\tUnderConId: %d\n", contractDetails.underConId);
	printf("\tLongName: %s\n", contractDetails.longName.c_str());
	printf("\tContractMonth: %s\n", contractDetails.contractMonth.c_str());
	printf("\tIndystry: %s\n", contractDetails.industry.c_str());
	printf("\tCategory: %s\n", contractDetails.category.c_str());
	printf("\tSubCategory: %s\n", contractDetails.subcategory.c_str());
	printf("\tTimeZoneId: %s\n", contractDetails.timeZoneId.c_str());
	printf("\tTradingHours: %s\n", contractDetails.tradingHours.c_str());
	printf("\tLiquidHours: %s\n", contractDetails.liquidHours.c_str());
	printf("\tEvRule: %s\n", contractDetails.evRule.c_str());
	printf("\tEvMultiplier: %g\n", contractDetails.evMultiplier);
	printf("\tAggGroup: %d\n", contractDetails.aggGroup);
	printf("\tUnderSymbol: %s\n", contractDetails.underSymbol.c_str());
	printf("\tUnderSecType: %s\n", contractDetails.underSecType.c_str());
	printf("\tMarketRuleIds: %s\n", contractDetails.marketRuleIds.c_str());
	printf("\tRealExpirationDate: %s\n", contractDetails.realExpirationDate.c_str());
	printf("\tLastTradeTime: %s\n", contractDetails.lastTradeTime.c_str());
	printContractDetailsSecIdList(contractDetails.secIdList);

}

void CIBSpi::printContractDetailsSecIdList(const TagValueListSPtr& secIdList)
{
	const int secIdListCount = secIdList.get() ? secIdList->size() : 0;
	if (secIdListCount > 0) {
		printf("\tSecIdList: {");
		for (int i = 0; i < secIdListCount; ++i) {
			const TagValue* tagValue = ((*secIdList)[i]).get();
			printf("%s=%s;", tagValue->tag.c_str(), tagValue->value.c_str());
		}
		printf("}\n");
	}

}

void CIBSpi::printBondContractDetailsMsg(const ContractDetails& contractDetails)
{
	printf("\tSymbol: %s\n", contractDetails.contract.symbol.c_str());
	printf("\tSecType: %s\n", contractDetails.contract.secType.c_str());
	printf("\tCusip: %s\n", contractDetails.cusip.c_str());
	printf("\tCoupon: %g\n", contractDetails.coupon);
	printf("\tMaturity: %s\n", contractDetails.maturity.c_str());
	printf("\tIssueDate: %s\n", contractDetails.issueDate.c_str());
	printf("\tRatings: %s\n", contractDetails.ratings.c_str());
	printf("\tBondType: %s\n", contractDetails.bondType.c_str());
	printf("\tCouponType: %s\n", contractDetails.couponType.c_str());
	printf("\tConvertible: %s\n", contractDetails.convertible ? "yes" : "no");
	printf("\tCallable: %s\n", contractDetails.callable ? "yes" : "no");
	printf("\tPutable: %s\n", contractDetails.putable ? "yes" : "no");
	printf("\tDescAppend: %s\n", contractDetails.descAppend.c_str());
	printf("\tExchange: %s\n", contractDetails.contract.exchange.c_str());
	printf("\tCurrency: %s\n", contractDetails.contract.currency.c_str());
	printf("\tMarketName: %s\n", contractDetails.marketName.c_str());
	printf("\tTradingClass: %s\n", contractDetails.contract.tradingClass.c_str());
	printf("\tConId: %ld\n", contractDetails.contract.conId);
	printf("\tMinTick: %g\n", contractDetails.minTick);
	printf("\tOrderTypes: %s\n", contractDetails.orderTypes.c_str());
	printf("\tValidExchanges: %s\n", contractDetails.validExchanges.c_str());
	printf("\tNextOptionDate: %s\n", contractDetails.nextOptionDate.c_str());
	printf("\tNextOptionType: %s\n", contractDetails.nextOptionType.c_str());
	printf("\tNextOptionPartial: %s\n", contractDetails.nextOptionPartial ? "yes" : "no");
	printf("\tNotes: %s\n", contractDetails.notes.c_str());
	printf("\tLong Name: %s\n", contractDetails.longName.c_str());
	printf("\tEvRule: %s\n", contractDetails.evRule.c_str());
	printf("\tEvMultiplier: %g\n", contractDetails.evMultiplier);
	printf("\tAggGroup: %d\n", contractDetails.aggGroup);
	printf("\tMarketRuleIds: %s\n", contractDetails.marketRuleIds.c_str());
	printf("\tTimeZoneId: %s\n", contractDetails.timeZoneId.c_str());
	printf("\tLastTradeTime: %s\n", contractDetails.lastTradeTime.c_str());
	printContractDetailsSecIdList(contractDetails.secIdList);

}


bool CIBSpi::TransOrderStatus(const std::string& ibOrderStatus, OrderStatus& orderStatus)
{
	//ApiCancelled -	准备撤单
	//ApiPending -		尚未达到IB服务器
	//PendingSubmit -	尚未到达交易所
	//PendingCancel -	尚未到达交易所
	//PreSubmitted -	模拟单已报
	//Submitted -		已报
	//Cancelled -		已撤单
	//Filled -			全部已成交
	//Inactive -		被拒绝，强制撤单

	if (ibOrderStatus == "ApiPending") return false;
	if (ibOrderStatus == "ApiCancelled") return false;

	if (ibOrderStatus == "PendingSubmit" || ibOrderStatus == "PreSubmitted" || ibOrderStatus == "Submitted")
	{
		orderStatus = OrderStatus::SomeDealed;
		return true;
	}
	if (ibOrderStatus == "Cancelled" || ibOrderStatus == "Inactive" || ibOrderStatus == "PendingCancel")
	{
		orderStatus = OrderStatus::AllCanceled;
		return true;
	}
	if (ibOrderStatus == "Filled")
	{
		orderStatus = OrderStatus::AllDealed;
		return true;
	}
	return false;
}

void CIBSpi::UpdateTickToApp(TickerId tickerId, IBTickPtr noMixCodeTick)
{
	MakeAndGet_Topic_OnTick()->UpdateTick_OnTick(noMixCodeTick);
}

void CIBSpi::TransBarTime(const std::string& bartime, std::string& timePart, std::string& timeZonePart)
{
	// 输入格式为 20240411 05:30:00 US/Eastern
	// 查询日线时，输入格式为 20240411
	// 
	// 返回的格式为 20240411 03:30:00
	std::vector<std::string> strs = CGlobal::SplitStr(bartime, " ");
	if (strs.size() < 1 || strs.size() > 3)
	{
		printf("bartime in wrong format %s \n", bartime.c_str());
		exit(-1);

	}
	if (strs.size() == 1)
	{
		timePart = strs[0];
		timeZonePart = "US/Eastern";
		return;

	}

	if (strs.size() == 2)
	{
		timePart = strs[0];
		timeZonePart = strs[1];
		return;
	}
	if (strs.size() == 3)
	{
		timePart = strs[0] + " " + strs[1];
		timeZonePart = strs[2];
		return;
	}
	return;
}

time_t CIBSpi::GetTimeByIBStr(const std::string& bartime, const std::string& timezonstr)
{
	// yyyymmdd
	int year = atoi(bartime.substr(0, 4).c_str());
	int mouth = atoi(bartime.substr(4, 2).c_str());
	int day = atoi(bartime.substr(6, 2).c_str());

	if (timezonstr.find("US/Eastern") != std::string::npos)
	{
		return CGlobal::GetTimeByStr(bartime);
	}
	if (timezonstr.find("US/Central") != std::string::npos)
	{
		return CGlobal::GetTimeByStr(bartime) + 60 * 60;
	}
	Log_Print(LogLevel::Err, fmt::format("unkown timezonstr: {}", timezonstr.c_str()));
	exit(-1);
	return 0;

}

