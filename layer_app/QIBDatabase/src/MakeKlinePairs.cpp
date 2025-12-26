#include "MakeKlinePairs.h"
#include "Factory.h"
#include <Global.h>
CMakeKlinePairs::CMakeKlinePairs(const std::string& first, const std::string& second, ITimeType timetype, const IQuery& query)
	:m_firstCodeId(first), m_secondCodeId(second), m_timeType(timetype), m_query(query)
{

}

IBTrader::IKLinePairs CMakeKlinePairs::DoMake()
{
	IKLinePairs ret;

	IKLines klines;
	MakeAndGet_Env()->GetDB_KLine()->GetKLines(m_firstCodeId, m_timeType, m_query, klines);
	for (const auto& kline : klines)
	{
		IKLinePair aPair;
		if (!DoOnePair(kline, aPair))
		{
			printf("can not make klinepair, %s_%s, %s\n", 
				m_firstCodeId.c_str(), 
				m_secondCodeId.c_str(),
				CGlobal::GetTickTimeStr(kline.time).c_str()
				
				);
			continue;
		}
		else
		{
			ret.push_back(aPair);
		}
	}
	return ret;

}

bool CMakeKlinePairs::DoOnePair(const IKLine& firstKline, IKLinePair& klinepair)
{
	IQuery query;
	query.byReqType = 2; // 2表示请求某个时间以前(包括该时间)多少个单位的数据(dwSubscribeNum为0时表示该时间前所有的数据)
	query.dwSubscribeNum = 1;
	query.tTime = firstKline.time;

	IKLines klines;
	MakeAndGet_Env()->GetDB_KLine()->GetKLines(m_secondCodeId, m_timeType, query, klines);
	if (klines.empty()) return false;

	klinepair.first = firstKline;
	klinepair.second = klines[0];
	return true;



}
