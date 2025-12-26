#include "pch.h"
#include "QCallbackObject_QGenerator.h"
#include <Factory_Log.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
#include <Global.h>
#include <Factory_HashEnv.h>
#include "Factory_Setup.h"
CQCallbackObject_QGenerator::CQCallbackObject_QGenerator()
	:m_lastUpdateSecond(0)
{

}

void CQCallbackObject_QGenerator::OnTick(IBTickPtr tick)
{
	// 更新行情数据库
	UpdateTickToDb(tick);

	// 驱动守护函数
	Get_DaemonByTick()->Execute();

}

void CQCallbackObject_QGenerator::UpdateTickToDb(IBTickPtr tick)
{
	std::map<CodeHashId, IBTickPtr>::iterator pos = m_lastUpdateTicks.find(tick->codeHash);
	if (pos != m_lastUpdateTicks.end())
	{
		if (pos->second->totalVol > tick->totalVol)
		{
			CodeStr codeId = Get_CodeIdEnv()->Get_CodeStrByHashId(tick->codeHash);
			std::string oldTimeStr = CGlobal::GetTickTimeStr(pos->second->time);
			std::string thisTimeStr = CGlobal::GetTickTimeStr(tick->time);
			std::string str = fmt::format("{} oldTime = {}, totalVol = {}, thisTime = {}, totalVol = {}",
				codeId.c_str(),
				oldTimeStr.c_str(), pos->second->totalVol,
				thisTimeStr.c_str(), tick->totalVol
			);
			Log_AsyncPrintDailyFile("totalvolErr", str, 1, false);
			return;
		}
	}
	m_lastUpdateTicks[tick->codeHash] = tick;


	MakeAndGet_QDatabase()->UpdateTicks(tick);

	//// 检查数据库最近更新的数据并打印到文件
	if (Get_CurrentTime()->GetCurrentTime_second() - m_lastUpdateSecond <= 10) return;

	int tickCount = 0;
	time_t recentUpdateSecnd = 0;
	IBTickPtr  lastUpdateTick = MakeAndGet_QDatabase()->GetLastUpdateTick(tickCount, recentUpdateSecnd);
	if (!lastUpdateTick->codeHash) return;

	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(tick->codeHash);
	if (!contract) return;

	std::string timeStr = Get_CurrentTime()->GetCurrentTimeStr();
	std::string tickTimeStr = CGlobal::GetTimeStr(lastUpdateTick->time / 1000);
	std::string recentUpdateSecndStr = CGlobal::GetTimeStr(recentUpdateSecnd);

	std::string temstr = fmt::format("totalCount = {}, lastTick is: {} {}, updated at: {} close={}",
		tickCount,
		Get_CodeIdEnv()->Get_CodeStrByHashId(lastUpdateTick->codeHash),
		tickTimeStr.c_str(),
		recentUpdateSecndStr.c_str(),
		lastUpdateTick->last * contract->minMove);

	int modSecond = 1;
	Log_AsyncPrintRotatingFile("tick.log", temstr, modSecond, true);
	m_lastUpdateSecond = Get_CurrentTime()->GetCurrentTime_second();

	return;

}
