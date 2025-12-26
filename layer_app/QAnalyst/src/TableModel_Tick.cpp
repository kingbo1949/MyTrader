#include "stdafx.h"
#include "TableModel_Tick.h"
#include <Factory_QDatabase.h>
#include <Log.h>
CTableModel_Tick::CTableModel_Tick(IbContractPtr pContract)
	:CTableModel(pContract)
{
}

void CTableModel_Tick::QueryData(const TimeZoneOfCodeId& timeZone)
{
	m_timeZone = timeZone;


	TimePair timePair;
	timePair.beginPos = timeZone.beginPos;
	timePair.endPos = timeZone.endPos;

	beginResetModel();
	m_ticks = MakeAndGet_QDatabase()->GetTickHisByLoop(timeZone.codeId, timePair);
	endResetModel();

}

void CTableModel_Tick::SaveData()
{
	std::string filename = GetFileName(m_timeZone.codeId);

	CLog::Instance()->DelLogFile(filename);

	IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(m_timeZone.codeId);

	for (size_t i = 0; i < m_ticks.size(); ++i)
	{
		std::string tickStr = GetTickStr(m_ticks[i], contract);

		CLog::Instance()->PrintStrToFile(filename, tickStr + "\n");
	}

}

int CTableModel_Tick::rowCount(const QModelIndex& parent) const
{
	return int(m_ticks.size());
}

QModelIndex CTableModel_Tick::nextBidJump(int beginRow)
{
	IBTickPtr lastTick = nullptr;
	IBTickPtr thisTick = nullptr;
	for (int row = beginRow; row < m_ticks.size(); ++row)
	{
		thisTick = m_ticks[row];
		if (row > 0)
		{
			lastTick = m_ticks[row - 1];
		}
		if (!IsJump_Bid(lastTick, thisTick)) continue;

		return index(row, 0);
	}
	return QModelIndex();
}

QModelIndex CTableModel_Tick::nextAskJump(int beginRow)
{
	IBTickPtr lastTick = nullptr;
	IBTickPtr thisTick = nullptr;
	for (int row = beginRow; row < m_ticks.size(); ++row)
	{
		thisTick = m_ticks[row];
		if (row > 0)
		{
			lastTick = m_ticks[row - 1];
		}
		if (!IsJump_Ask(lastTick, thisTick)) continue;

		return index(row, 0);
	}
	return QModelIndex();
}

std::string CTableModel_Tick::GetFileName(const CodeStr& codeId)
{
	std::string filename = codeId + "_tick.csv";
	return filename;
}
