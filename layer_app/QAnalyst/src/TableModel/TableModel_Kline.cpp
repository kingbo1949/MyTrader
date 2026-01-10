#include "../stdafx.h"
#include "TableModel_Kline.h"
#include <Factory_QDatabase.h>
#include <Log.h>
#include <Factory_IBGlobalShare.h>
#include <Global.h>
#include <QtGlobal.h>
#include "../SaveData_Klines.h"
CTableModel_Kline::CTableModel_Kline(IbContractPtr pContract)
	:CTableModel(pContract)
{
	m_headers = { "Time", "Open", "High", "Low", "Close", "Vol", "Dif", "Dea", "Macd", "DivType", "IsUTurn", "Atr" };
}

void CTableModel_Kline::QueryData(const TimeZoneOfCodeId& timeZone)
{
	m_timeZone = timeZone;

	QQuery qQuery;
	qQuery.query_type = QQueryType::BETWEEN_TIME;
	qQuery.time_pair.beginPos = timeZone.beginPos;
	qQuery.time_pair.endPos = timeZone.endPos;

	beginResetModel();
	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLineByLoop(timeZone.codeId, timeZone.timeType, qQuery.time_pair);
	FillKline4Tables(timeZone.codeId, timeZone.timeType, klines);
	endResetModel();

}

void CTableModel_Kline::SaveData()
{
	std::string filename = GetFileName(m_timeZone.codeId);
	CSaveData_Klines saveData(m_timeZone.codeId, m_kline4Tables);
	saveData.Go(filename);
}

int CTableModel_Kline::rowCount(const QModelIndex& parent) const
{
	return int(m_kline4Tables.size());
}

QVariant CTableModel_Kline::data(const QModelIndex& index, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();

	//IBKLinePtr kline = m_klines[index.row()];
	KLine4Table kline4Table = m_kline4Tables[index.row()];

	KLineRole klineRole = static_cast<KLineRole>(index.column());
	if (klineRole == KLineRole::Time)
	{
		std::string timestr = CGlobal::GetTickTimeStr(kline4Table.pkline->time).c_str();
		return CQtGlobal::StdString_QString(timestr);

	}
	if (klineRole == KLineRole::Open)
	{
		return QString::number(kline4Table.pkline->open, 'f', 2);
	}
	if (klineRole == KLineRole::High)
	{
		return QString::number(kline4Table.pkline->high, 'f', 2);
	}
	if (klineRole == KLineRole::Low)
	{
		return QString::number(kline4Table.pkline->low, 'f', 2);
	}
	if (klineRole == KLineRole::Close)
	{
		return QString::number(kline4Table.pkline->close, 'f', 2);
	}
	if (klineRole == KLineRole::Vol)
	{
		return QString::number(kline4Table.pkline->vol);
	}
	if (klineRole == KLineRole::Dif)
	{
		return QString::number(kline4Table.pmacd->dif, 'f', 2);
	}
	if (klineRole == KLineRole::Dea)
	{
		return QString::number(kline4Table.pmacd->dea, 'f', 2);
	}
	if (klineRole == KLineRole::Macd)
	{
		return QString::number(kline4Table.pmacd->macd, 'f', 2);
	}
	if (klineRole == KLineRole::DivType)
	{
		std::string str = CTransToStr::Get_DivergenceType(kline4Table.pDivType->divType);
		return CQtGlobal::StdString_QString(str);
	}
	if (klineRole == KLineRole::IsUTurn)
	{
		return kline4Table.pDivType->isUTurn ? "1" : "0";
	}
	if (klineRole == KLineRole::Atr)
	{
		return QString::number(kline4Table.pAtr->avgAtr, 'f', 2);
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

void CTableModel_Kline::FillKline4Tables(const CodeStr& codeId, Time_Type timeType, const IBKLinePtrs &klines)
{
	m_kline4Tables.clear();
	for (auto kline : klines)
	{
		KLine4Table kLine4Table;
		kLine4Table.pkline = kline;
		kLine4Table.pmacd = MakeAndGet_QDatabase()->GetOneMacd(codeId, timeType, kline->time);
		kLine4Table.pDivType = MakeAndGet_QDatabase()->GetOneDivType(codeId, timeType, kline->time);
		kLine4Table.pAtr = MakeAndGet_QDatabase()->GetOneAtr(codeId, timeType, kline->time);
		m_kline4Tables.push_back(kLine4Table);
	}
	return ;
}
