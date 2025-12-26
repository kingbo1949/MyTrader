#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <future>
#include <string.h>
#include <iterator>

typedef std::shared_ptr<std::thread> C11ThreadPtr;

const std::string MonitorLog = "monitor.log";
const std::string TraderIdSimulaot = "88888888";
const size_t CodeHashIdSize = 100;
const size_t StrategyHashIdSize = 100;

const size_t MaxPriceRowCountSubscribe = 10;	// 订阅行情时的最大行情深度
const size_t MaxPriceRowCountDb = 5;			// 数据库中保存的最大行情深度

typedef std::string CodeStr;
typedef std::vector<CodeStr> CodeStrs;
typedef size_t CodeHashId;


typedef std::string StrategyId;			// 策略id由三个部分拼接而成： 策略名称 + 合约 + 多空 + 识别码
typedef std::vector<StrategyId> StrategyIds;
typedef size_t StrategyIdHashId;

typedef int LocalOrderNo;








