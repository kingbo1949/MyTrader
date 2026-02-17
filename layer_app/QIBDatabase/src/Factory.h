#pragma once

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <MyThreadPool.h>

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

MyThreadPoolPtr				MakeAndGet_MyThreadPool();

void 						GetKline_RecountQuery_All(const std::string& codeId, ITimeType timetype, IKLines& klines);
void						GetKline_RecountQuery_TimePos(const std::string& codeId, ITimeType timetype, long long int timePos, IKLines& klines);
std::string					GetKlineStr(const IKLine& kline);

