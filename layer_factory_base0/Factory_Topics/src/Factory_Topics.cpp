#include "pch.h"
#include "Factory_Topics.h"

#include "Topic_OnTick_Real.h"
#include "Topic_OnTrade_Real.h"

Topic_OnTickPtr		g_topic_ontick = nullptr;
FACTORY_TOPIC_API Topic_OnTickPtr MakeAndGet_Topic_OnTick()
{
	if (!g_topic_ontick)
	{
		g_topic_ontick = std::make_shared<CTopic_OnTick_Real>();
	}
	return g_topic_ontick;
}

Topic_OnTradePtr	g_topic_ontrade = nullptr;
FACTORY_TOPIC_API Topic_OnTradePtr MakeAndGet_Topic_OnTrade()
{
	if (!g_topic_ontrade)
	{
		g_topic_ontrade = std::make_shared<CTopic_OnTrade_Real>();
	}
	return g_topic_ontrade;
}
