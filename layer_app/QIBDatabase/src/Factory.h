#pragma once

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>

using namespace IceUtil;
using namespace Ice;

#include "Env_IB.h"
#include "TimerTask_UpdateIndex.h"

Env_IBPtr					MakeAndGet_Env();

void						SetCommunicator(CommunicatorPtr pCommunicator);
CommunicatorPtr				GetCommunicator();

std::string					Trans_Str(ITimeType timetype);

// 检查Kline的合法性
bool						ValidKline(const std::string& codeId, ITimeType timetype, const IKLine& kline);

TimerTask_UpdateIndexPtr	MakeAndGet_TimerTask_UpdateIndex();
