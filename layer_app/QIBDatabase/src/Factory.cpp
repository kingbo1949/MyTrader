#include "pch.h"
#include "Factory.h"




Env_IBPtr g_pEnvIb = nullptr;
Env_IBPtr MakeAndGet_Env()
{
	if (!g_pEnvIb)
	{
		PropertiesPtr properties = createProperties();
		properties->load(CONFIG_FILE);
		int sizeG = properties->getPropertyAsInt("QDatabaseHis.SizeG");
		int sizeM = properties->getPropertyAsInt("QDatabaseHis.SizeM");

		g_pEnvIb = std::make_shared<CEnv_IB>(".\\", "history", sizeG, sizeM, 400000, 100000);
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
