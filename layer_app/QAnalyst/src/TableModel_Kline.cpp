#include "stdafx.h"
#include "TableModel_Kline.h"
#include <Factory_QDatabase.h>
#include <Log.h>
#include <Factory_IBGlobalShare.h>
#include <Global.h>
#include <QtGlobal.h>
#include "SaveData_Klines.h"
CTableModel_Kline::CTableModel_Kline(IbContractPtr pContract)
	:CTableModel(pContract)
{
	m_headers = { "Time", "Open", "High", "Low", "Close", "Vol" };
}

void CTableModel_Kline::QueryData(const TimeZoneOfCodeId& timeZone)
{
	m_timeZone = timeZone;

	QQuery qQuery;
	qQuery.query_type = QQueryType::BETWEEN_TIME;
	qQuery.time_pair.beginPos = timeZone.beginPos;
	qQuery.time_pair.endPos = timeZone.endPos;

	beginResetModel();
	m_klines = MakeAndGet_QDatabase()->GetKLineByLoop(timeZone.codeId, timeZone.timeType, qQuery.time_pair);
	endResetModel();

}

void CTableModel_Kline::SaveData()
{
	std::string filename = GetFileName(m_timeZone.codeId);
	CSaveData_Klines saveData(m_timeZone.codeId, m_klines);
	saveData.Go(filename);
}

int CTableModel_Kline::rowCount(const QModelIndex& parent) const
{
	return int(m_klines.size());
}

QVariant CTableModel_Kline::data(const QModelIndex& index, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();

	IBKLinePtr kline = m_klines[index.row()];

	KLineRole klineRole = static_cast<KLineRole>(index.column());
	if (klineRole == KLineRole::Time)
	{
		std::string timestr = CGlobal::GetTickTimeStr(kline->time).c_str();
		return CQtGlobal::StdString_QString(timestr);

	}
	if (klineRole == KLineRole::Open)
	{
		return QString::number(kline->open, 'f', 2);
	}
	if (klineRole == KLineRole::High)
	{
		return QString::number(kline->high, 'f', 2);
	}
	if (klineRole == KLineRole::Low)
	{
		return QString::number(kline->low, 'f', 2);
	}
	if (klineRole == KLineRole::Close)
	{
		return QString::number(kline->close, 'f', 2);
	}
	if (klineRole == KLineRole::Vol)
	{
		return QString::number(kline->vol);
	}
	return QVariant();
}

std::string CTableModel_Kline::GetFileName(const CodeStr& codeId)
{
	std::string filename = fmt::format("{}_kline_{}.csv",
		codeId.c_str(), CTransToStr::Get_TimeType(m_timeZone.timeType)
	);
	return filename;
}
