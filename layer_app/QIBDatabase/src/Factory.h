#pragma once

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>

using namespace IceUtil;
using namespace Ice;

#include "Env_IB.h"
#include "TimerTask_UpdateIndex.h"

// 所有时间周期枚举值，用于启动阶段预建列族
constexpr ITimeType ALL_TIME_TYPES[] =
{
	ITimeType::S15, ITimeType::M1, ITimeType::M5,
	ITimeType::M15, ITimeType::M30, ITimeType::H1, ITimeType::D1
};

Env_IBPtr					MakeAndGet_Env();

void						SetCommunicator(CommunicatorPtr pCommunicator);
CommunicatorPtr				GetCommunicator();

std::string					Trans_Str(ITimeType timetype);

// 检查Kline的合法性
bool						ValidKline(const std::string& codeId, ITimeType timetype, const IKLine& kline);

TimerTask_UpdateIndexPtr	MakeAndGet_TimerTask_UpdateIndex();
