#pragma once

#include "Factory_Topics_Export.h"

// 模块任务描述：
// 1、行情主题 行情发生器会把tick发送给主题，而需要行情tick的模块也需要到这里订阅行情
// 2、交易主题 委托单的状态改变等信息会被发送给主题，而需要监控委托单状态的模块也需要到这里来订阅委托状态信息



#include "Topic_OnTick.h"
#include "Topic_OnTrade.h"


	FACTORY_TOPIC_API Topic_OnTickPtr		MakeAndGet_Topic_OnTick();

	FACTORY_TOPIC_API Topic_OnTradePtr		MakeAndGet_Topic_OnTrade();






