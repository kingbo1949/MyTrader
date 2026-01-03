#include "pch.h"
#include "IceTransfor.h"
#include "Factory_QDatabase.h"
#include "Factory_HashEnv.h"
#include <Global.h>
#include <Factory_Log.h>
IBTickPtr CIceTransfor::TransTickIceToMy(const ITick& tick)
{
	CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(tick.codeId.c_str());
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeHash);
	IBTickPtr back = std::make_shared<CIBTick>();

	back->codeHash = codeHash;
	back->time = tick.time;
	for (auto bidask : tick.bidAsks)
	{
		IBBidAsk ib_bidask;
		ib_bidask.bid = CGlobal::DToI(bidask.bid / contract->minMove);
		ib_bidask.bidVol = bidask.bidVol;
		ib_bidask.ask = CGlobal::DToI(bidask.ask / contract->minMove);
		ib_bidask.askVol = bidask.askVol;

		back->bidAsks.push_back(ib_bidask);
	}
	back->open = CGlobal::DToI(tick.open / contract->minMove);
	back->last = CGlobal::DToI(tick.last / contract->minMove);
	back->vol = tick.vol;
	back->totalVol = tick.totalVol;
	back->turnOver = tick.turnOver;
	back->timeStamp = tick.timeStamp;

	return back;

}


IBTrader::ITick CIceTransfor::TransTickMyToIce(IBTickPtr tick)
{
	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(tick->codeHash);

	ITick back;
	back.codeId = Get_CodeIdEnv()->Get_CodeStrByHashId(tick->codeHash);
	back.time = tick->time;
	for (auto bidask : tick->bidAsks)
	{
		IBidAsk ibidask;
		ibidask.bid = bidask.bid * contract->minMove;
		ibidask.bidVol = bidask.bidVol;
		ibidask.ask = bidask.ask * contract->minMove;
		ibidask.askVol = bidask.askVol;
		back.bidAsks.push_back(ibidask);
	}
	back.open = tick->open * contract->minMove;
	back.last = tick->last * contract->minMove;
	back.vol = tick->vol;
	back.totalVol = tick->totalVol;
	back.turnOver = tick->turnOver;
	back.timeStamp = tick->timeStamp;
	return back;
}

IBTrader::IQuery CIceTransfor::TransQueryMyToIce(const QQuery& query)
{
	IQuery back;
	if (query.query_type == QQueryType::ALL_DATA)
	{
		back.byReqType = 1;
	}
	if (query.query_type == QQueryType::FROM_CURRENT)
	{
		back.byReqType = 0;
		back.dwSubscribeNum = query.query_number;
	}
	if (query.query_type == QQueryType::BEFORE_TIME)
	{
		back.byReqType = 2;
		back.dwSubscribeNum = query.query_number;
		back.tTime = query.time_ms;
	}
	if (query.query_type == QQueryType::AFTER_TIME)
	{
		back.byReqType = 3;
		back.dwSubscribeNum = query.query_number;
		back.tTime = query.time_ms;
	}
	if (query.query_type == QQueryType::BETWEEN_TIME)
	{
		back.byReqType = 4;
		back.timePair.beginPos = query.time_pair.beginPos;
		back.timePair.endPos = query.time_pair.endPos;

	}


	return back;

}

IBKLinePtr CIceTransfor::TransKLineIceToMy(const IKLine& kline)
{
	IBKLinePtr back = std::make_shared<CIBKLine>();

	back->time = kline.time;
	back->open = kline.open;
	back->close = kline.close;
	back->high = kline.high;
	back->low = kline.low;
	back->vol = int(kline.vol);
	return back;


}

IBTrader::IKLine CIceTransfor::TransKLineMyToIce(const IBKLinePtr& kline)
{
	IKLine back;
	back.time = kline->time;
	back.open = kline->open;
	back.close = kline->close;
	back.high = kline->high;
	back.low = kline->low;
	back.vol = kline->vol;
	return back;


}

IBTrader::ITimeType CIceTransfor::TransTimeTypeToIce(Time_Type timeType)
{
	ITimeType back = ITimeType::M1;

	switch (timeType)
	{
	case Time_Type::S15:
		back = ITimeType::S15;
		break;
	case Time_Type::M1:
		back = ITimeType::M1;
		break;
	case Time_Type::M5:
		back = ITimeType::M5;
		break;
	case Time_Type::M15:
		back = ITimeType::M15;
		break;
	case Time_Type::M30:
		back = ITimeType::M30;
		break;
	case Time_Type::H1:
		back = ITimeType::H1;
		break;
	case Time_Type::D1:
		back = ITimeType::D1;
		break;
	default:
		printf("unknown timetype \n");
		exit(-1);
		break;

	}

	return back;
}

IBAvgValuePtr CIceTransfor::TransAvgValueIceToMy(const IAvgValue& value)
{
	IBMaPtr ret = std::make_shared<CIBAvgValue>();
	ret->time = value.time;
	ret->v5 = value.v5;
	ret->v20 = value.v20;
	ret->v60 = value.v60;
	ret->v200 = value.v200;
	return ret;
}

IBTrader::IAvgValue CIceTransfor::TransAvgValueMyToIce(const IBAvgValuePtr& ma)
{
	IAvgValue ret;
	ret.time = ma->time;
	ret.v5 = ma->v5;
	ret.v20 = ma->v20;
	ret.v60 = ma->v60;
	ret.v200 = ma->v200;
	return ret;
}

IBKLinePair CIceTransfor::TransKLinePairIceToMy(const IKLinePair& klinePair)
{
	IBKLinePair ret;
	ret.first = TransKLineIceToMy(klinePair.first);
	ret.second = TransKLineIceToMy(klinePair.second);
	return ret;
}

IBTrader::IKLinePair CIceTransfor::TransKLinePairMyToIce(const IBKLinePair& klinePair)
{
	IKLinePair ret;
	ret.first = TransKLineMyToIce(klinePair.first);
	ret.second = TransKLineMyToIce(klinePair.second);
	return ret;
}

IBMacdPtr CIceTransfor::TransMacd(const IMacdValue& value)
{
	IBMacdPtr back = std::make_shared<CIBMacd>();

	back->time = value.time;
	back->emaShort = value.emaShort;
	back->emaLong = value.emaLong;
	back->dif = value.dif;
	back->dea = value.dea;
	back->macd = value.macd;
	return back;
}

IMacdValue CIceTransfor::TransMacd(const IBMacdPtr pValue)
{
	IMacdValue back;
	back.emaShort = pValue->emaShort;
	back.emaLong = pValue->emaLong;
	back.dif = pValue->dif;
	back.dea = pValue->dea;
	back.macd = pValue->macd;
	return back;
}

IBDivTypePtr CIceTransfor::TransDivType(const IDivTypeValue& value)
{
	IBDivTypePtr back = std::make_shared<CIBDivType>();

	back->time = value.time;
	if (value.divType == IBTrader::IDivType::NORMAL)
	{
		back->divType = DivergenceType::Normal;
	}
	if (value.divType == IBTrader::IDivType::TOP)
	{
		back->divType = DivergenceType::Top;
	}
	if (value.divType == IBTrader::IDivType::BOTTOM)
	{
		back->divType = DivergenceType::Bottom;
	}
	if (value.divType == IBTrader::IDivType::BOTTOMSUB)
	{
		back->divType = DivergenceType::BottomSub;
	}
	if (value.divType == IBTrader::IDivType::TOPSUB)
	{
		back->divType = DivergenceType::TopSub;
	}
	back->isUTurn = value.isUTurn;
	return back;
}

IDivTypeValue CIceTransfor::TransDivType(IBDivTypePtr pValue)
{
	IDivTypeValue back;
	back.time = pValue->time;
	if (pValue->divType == DivergenceType::Normal)
	{
		back.divType = IBTrader::IDivType::NORMAL;
	}
	if (pValue->divType == DivergenceType::Top)
	{
		back.divType = IBTrader::IDivType::TOP;
	}
	if (pValue->divType == DivergenceType::Bottom)
	{
		back.divType = IBTrader::IDivType::BOTTOM;
	}
	if (pValue->divType == DivergenceType::TopSub)
	{
		back.divType = IBTrader::IDivType::TOPSUB;
	}
	if (pValue->divType == DivergenceType::BottomSub)
	{
		back.divType = IBTrader::IDivType::BOTTOMSUB;
	}
	back.isUTurn = pValue->isUTurn;
	return back;
}

IBAtrPtr CIceTransfor::TransAtr(const IAtrValue &value)
{
	IBAtrPtr back = std::make_shared<CIBAtr>();

	back->time = value.time;
	back->thisAtr = value.thisAtr;
	back->avgAtr = value.avgAtr;
	return back;

}

IAtrValue CIceTransfor::TransAtr(const IBAtrPtr& pValue)
{
	IAtrValue back;
	back.time = pValue->time;
	back.thisAtr = pValue->thisAtr;
	back.avgAtr = pValue->avgAtr;
	return back;

}
