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
	m_headers = { "Time", "Open", "High", "Low", "Close", "Vol", "Dif", "Dea", "Macd", "DivType", "IsUTurn", "Atr", "Ma5", "Ma20", "Ma60", "Ma200", "PreDayHigh", "PreDayLow"  };
}

void CTableModel_Kline::QueryData(const TimeZoneOfCodeId& timeZone)
{
	m_timeZone = timeZone;

	QQuery qQuery;
	qQuery.query_type = QQueryType::BETWEEN_TIME;
	qQuery.time_pair.beginPos = timeZone.beginPos;
	qQuery.time_pair.endPos = timeZone.endPos;

	beginResetModel();
	m_richDatas = MakeAndGet_QDatabase()->GetRichsByLoop(timeZone.codeId, timeZone.timeType, qQuery.time_pair);
	// IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLineByLoop(timeZone.codeId, timeZone.timeType, qQuery.time_pair);
	// FillKline4Tables(timeZone.codeId, timeZone.timeType, klines);
	endResetModel();

}

void CTableModel_Kline::SaveData()
{
	std::string filename = GetFileName(m_timeZone.codeId);
	CSaveData_Klines saveData(m_timeZone.codeId, m_richDatas);
	saveData.Go(filename);
}

int CTableModel_Kline::rowCount(const QModelIndex& parent) const
{
	return int(m_richDatas.size());
}

QVariant CTableModel_Kline::data(const QModelIndex& index, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();

	IBRichDataPtr pRichData = m_richDatas[index.row()];

	KLineRole klineRole = static_cast<KLineRole>(index.column());
	if (klineRole == KLineRole::Time)
	{
		std::string timestr = CGlobal::GetTickTimeStr(pRichData->time).c_str();
		return CQtGlobal::StdString_QString(timestr);

	}
	if (klineRole == KLineRole::Open)
	{
		return QString::number(pRichData->open, 'f', 2);
	}
	if (klineRole == KLineRole::High)
	{
		return QString::number(pRichData->high, 'f', 2);
	}
	if (klineRole == KLineRole::Low)
	{
		return QString::number(pRichData->low, 'f', 2);
	}
	if (klineRole == KLineRole::Close)
	{
		return QString::number(pRichData->close, 'f', 2);
	}
	if (klineRole == KLineRole::Vol)
	{
		return QString::number(pRichData->vol);
	}
	if (klineRole == KLineRole::Dif)
	{
		return QString::number(pRichData->dif, 'f', 2);
	}
	if (klineRole == KLineRole::Dea)
	{
		return QString::number(pRichData->dea, 'f', 2);
	}
	if (klineRole == KLineRole::Macd)
	{
		return QString::number(pRichData->macd, 'f', 2);
	}
	if (klineRole == KLineRole::DivType)
	{
		std::string str = CTransToStr::Get_DivergenceType(pRichData->divType);
		return CQtGlobal::StdString_QString(str);
	}
	if (klineRole == KLineRole::IsUTurn)
	{
		return pRichData->isUTurn ? "1" : "0";
	}
	if (klineRole == KLineRole::Atr)
	{
		return QString::number(pRichData->avgAtr, 'f', 2);
	}
	if (klineRole == KLineRole::Ma5)
	{
		return QString::number(pRichData->ma5, 'f', 2);
	}
	if (klineRole == KLineRole::Ma20)
	{
		return QString::number(pRichData->ma20, 'f', 2);
	}
	if (klineRole == KLineRole::Ma60)
	{
		return QString::number(pRichData->ma60, 'f', 2);
	}
	if (klineRole == KLineRole::Ma200)
	{
		return QString::number(pRichData->ma200, 'f', 2);
	}

	if (klineRole == KLineRole::PreDayHigh)
	{
		return QString::number(pRichData->preDayHigh, 'f', 2);
	}
	if (klineRole == KLineRole::PreDayLow)
	{
		return QString::number(pRichData->preDayLow, 'f', 2);
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

// void CTableModel_Kline::FillKline4Tables(const CodeStr& codeId, Time_Type timeType, const IBKLinePtrs &klines)
// {
// 	m_kline4Tables.clear();
// 	for (auto kline : klines)
// 	{
// 		KLine4Table kLine4Table;
// 		kLine4Table.pkline = kline;
// 		kLine4Table.pmacd = MakeAndGet_QDatabase()->GetOneMacd(codeId, timeType, kline->time);
// 		kLine4Table.pDivType = MakeAndGet_QDatabase()->GetOneDivType(codeId, timeType, kline->time);
// 		kLine4Table.pAtr = MakeAndGet_QDatabase()->GetOneAtr(codeId, timeType, kline->time);
// 		kLine4Table.pMa = MakeAndGet_QDatabase()->GetOneMa(codeId, timeType, kline->time);
// 		m_kline4Tables.push_back(kLine4Table);
// 	}
// 	return ;
// }
