#pragma once

#include <memory>
#include <base_struc.h>

// 行情更新回调函数指针
typedef void(*PFunc_OnTick)(IBTickPtr tick);

// 交易更新回调函数指针
// 交易回调不需要返回更新的原因，底层状态已经改变，只需要驱动应用层去运行，应用层自然会查询到目前的状态
typedef void(*PFunc_OnTrade)(StrategyIdHashId strategyIdHashId);