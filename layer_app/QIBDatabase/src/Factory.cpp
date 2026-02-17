#include "pch.h"
#include "Factory.h"
#include <Global.h>
#include <Factory_Log.h>


Env_IBPtr g_pEnvIb = nullptr;
Env_IBPtr MakeAndGet_Env()
{
	if (!g_pEnvIb)
	{
		g_pEnvIb = std::make_shared<CEnv_IB>("history_rocksdb");
	}
	return g_pEnvIb;
}

CommunicatorPtr g_pCommunicator = NULL;
void SetCommunicator(CommunicatorPtr pCommunicator)
{
	if (!g_pCommunicator)
	{
		g_pCommunicator = pCommunicator;
	}
	return;
}

CommunicatorPtr GetCommunicator()
{
	return g_pCommunicator;
}

std::string Trans_Str(ITimeType timetype)
{
	std::string back = "";
	switch (timetype)
	{
	case ITimeType::S15:
		back = "S15";
		break;
	case ITimeType::M1:
		back = "M1";
		break;
	case ITimeType::M5:
		back = "M5";
		break;
	case ITimeType::M15:
		back = "M15";
		break;
	case ITimeType::M30:
		back = "M30";
		break;
	case ITimeType::H1:
		back = "H1";
		break;
	case ITimeType::D1:
		back = "D1";
		break;
	default:
		printf("unknown timetype \n");
		exit(-1);
		break;
	}
	return back;
}

bool ValidKline(const std::string& codeId, ITimeType timetype, const IKLine& kline)
{
	// 成交量不为0
	if (kline.vol == 0)
	{
		//printf("invalid kline, %s vol == 0, %s, %s \n", codeId.c_str(), Trans_Str(timetype).c_str(), CGlobal::GetTickTimeStr(kline.time).c_str());
		return false;
	}
	return true;
}

TimerTask_UpdateIndexPtr g_pTimerTask_UpdateIndex = nullptr;
TimerTask_UpdateIndexPtr MakeAndGet_TimerTask_UpdateIndex()
{
	if (!g_pTimerTask_UpdateIndex)
	{
		g_pTimerTask_UpdateIndex = std::make_shared<CTimerTask_UpdateIndex>();
	}
	return g_pTimerTask_UpdateIndex;
}


MyThreadPoolPtr g_pThreadPool = NULL;
MyThreadPoolPtr MakeAndGet_MyThreadPool()
{
	if (!g_pThreadPool)
	{
		const int threadSize = 4;
		g_pThreadPool = std::make_shared<CMyThreadPool>(threadSize);
	}
	return g_pThreadPool;
}

void GetKline_RecountQuery_All(const std::string& codeId, ITimeType timetype, IKLines& klines)
{
	// 在数据库内部，没有经过ICE，所以不需要接力
	IQuery query;
	query.byReqType = 1;	// 查询全部数据

	MakeAndGet_Env()->GetDB_KLine()->GetKLines(codeId, timetype, query, klines);

	return;
}

void GetKline_RecountQuery_TimePos(const std::string& codeId, ITimeType timetype, long long int timePos, IKLines& klines)
{
	// 在数据库内部，没有经过ICE，所以不需要接力
	IQuery query;
	query.byReqType = 3;		// 查询某时间点之后
	query.tTime = timePos;
	query.dwSubscribeNum = LLONG_MAX;	// 查询某时点之后全部数据
	MakeAndGet_Env()->GetDB_KLine()->GetKLines(codeId, timetype, query, klines);
	return;
}

std::string GetKlineStr(const IKLine &kline)
{
	std::string temstr = fmt::format(
	"{},"					    // 时间
	"{:.2f},"					// 开盘
	"{:.2f},"					// 最高
	"{:.2f},"					// 最低
	"{:.2f},"					// close
	"{},"						// vol
	,
	CGlobal::GetTickTimeStr(kline.time).substr(0, 17).c_str(),			// 取时间字符串不要毫秒部分
	kline.open,
	kline.high,
	kline.low,
	kline.close,
	kline.vol
	);

	return temstr;

}
