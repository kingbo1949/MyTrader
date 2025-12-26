#pragma once

#include <string>


const std::string BreakInDayName = "breakinday";				// 日内20分钟突破，跟踪止盈止损，收盘平仓

const std::string ManualName = "manual";						// 

const std::string On29minName = "on29min";					// 如果本30分钟k线突破，则在29分钟时开仓

const std::string DoubleBreakName = "doubleBreak";			// 主周期是M30, 当前BAR结束前3分钟才可以开仓，开仓条件是要突破最近N根30分钟线最高价

const std::string JumpTickName = "jumpTick";					// 高频交易主周期是TICK

const std::string DayBreakName = "dayBreak";					// 以开盘价格为基线，往上开多头，往下开空头，收盘平仓
const std::string DayBreakManualName = "dayBreakManual";		// 手动以指定价格为基线，往上开多头，往下开空头，收盘平仓





