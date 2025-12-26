#include "pch.h"
#include "Factory_Monitor.h"
#include "Monitor_Real.h"

MonitorPtr g_pMonitor = nullptr;
FACTORY_MONITOR_API MonitorPtr MakeAndGet_Monitor()
{
	if (!g_pMonitor)
	{
		g_pMonitor = std::make_shared<CMonitor_Real>();
	}
	return g_pMonitor;
}
