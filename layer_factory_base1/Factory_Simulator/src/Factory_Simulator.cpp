#include "pch.h"
#include "Factory_Simulator.h"

#include "Simulator_Real.h"
#include "MeetDeal_Loose.h"
#include "MeetDeal_Rigorous.h"
SimulatorPtr g_pSimulator = nullptr;
FACTORY_SIMULATOR_API SimulatorPtr MakeAndGet_Simulator()
{
	if (!g_pSimulator)
	{
		g_pSimulator = std::make_shared<CSimulator_Real>();
	}
	return g_pSimulator;
}

MeetDealType g_meetDealType = MeetDealType::loose;
MeetDealPtr g_pMeetDeal = nullptr;
FACTORY_SIMULATOR_API MeetDealPtr MakeAndGet_MeetDeal()
{
	if (!g_pMeetDeal)
	{
		if (g_meetDealType == MeetDealType::loose)
		{
			g_pMeetDeal = std::make_shared<CMeetDeal_Loose>();
		}
		else
		{
			g_pMeetDeal = std::make_shared<CMeetDeal_Rigorous>();
		}
	}
	return g_pMeetDeal;
}



