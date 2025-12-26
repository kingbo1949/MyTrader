#include "pch.h"
#include "DaemonByTick_QGenerator.h"
#include "CmdUpdateDbFromIB.h"
#include <Factory_IBGlobalShare.h>
#include <Global.h>
CDaemonByTick_QGenerator::CDaemonByTick_QGenerator()
	:m_lastUpdateDbSeccond(0)
{
}
void CDaemonByTick_QGenerator::Execute()
{
	Handle_Log();

	// 每90分钟做一次更新
	if (Get_CurrentTime()->GetCurrentTime_second() - m_lastUpdateDbSeccond < 90 * 60) return;

	time_t now = Get_CurrentTime()->GetCurrentTime_second();
	tm tt;
	if (!CGlobal::MyLocalTime(&now, &tt)) return;

	// 避免碰巧在整数点更新数据
	if (tt.tm_min != 13 && tt.tm_min != 43) return;

	CCmdUpdateDbFromIB cmd(UseType::QGenerator);
	MakeAndGet_MyThreadPool()->commit(cmd);

	m_lastUpdateDbSeccond = Get_CurrentTime()->GetCurrentTime_second();

}

void CDaemonByTick_QGenerator::UpdateDbSecond()
{
	m_lastUpdateDbSeccond = Get_CurrentTime()->GetCurrentTime_second();
	return;
}

DaemonByTick_QGeneratorPtr g_pDaemonByTick_QGenerator = nullptr;
DaemonByTick_QGeneratorPtr MakeAndGet_DaemonByTick_QGenerator()
{
	if (!g_pDaemonByTick_QGenerator)
	{
		g_pDaemonByTick_QGenerator = std::make_shared<CDaemonByTick_QGenerator>();
	}
	return g_pDaemonByTick_QGenerator;
}
