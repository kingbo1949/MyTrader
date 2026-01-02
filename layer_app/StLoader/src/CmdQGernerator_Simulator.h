#pragma once

#include <memory>
#include <string>
#include <base_struc.h>
#include <Factory_AnalystDeal.h>

// 从数据库取行情数据，作为数据源，推送到主题器
class CCmdQGernerator_Simulator
{
public:
	CCmdQGernerator_Simulator();
	virtual ~CCmdQGernerator_Simulator() { ; };

	void					operator()();

	std::string				GetThreadName();

protected:
	std::string				m_filepath;
	IBTickPtrs				GetTicksFromDb_Ticks(const TimeZoneOfCodeId& timeZoneOfCodeId);

	IBTickPtrs				GetTicksFromDb_Klines(const TimeZoneOfCodeId& timeZoneOfCodeId);

	void					PrintTicksDec(const IBTickPtrs& ticks);

	// 打印交易分析结果
	void					PrintAnalystResult();

	// 将timeType转变为毫秒
	int						TransTimeTypeToPos(Time_Type timeType);

	// 按月打印分析
	void					PrintByMonth(const SimpleMatchPtrs& totalMatches);

};
typedef std::shared_ptr<CCmdQGernerator_Simulator> CmdQGernerator_SimulatorPtr;

