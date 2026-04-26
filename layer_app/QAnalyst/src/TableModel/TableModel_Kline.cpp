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
	m_headers = {
		"Time", "Open", "High", "Low", "Close", "Vol", "Dif", "Dea", "Macd", "DivType", "IsUTurn", "Atr", "Ma5", "Ma20", "Ma60", "Ma200", "PreDayHigh", "PreDayLow", "PreDayClose",
		"ITime", "IOpen","IHigh", "ILow", "IClose", "IVol", "IDif", "IDea", "IMacd", "IDivType", "IIsUTurn"
	};
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
	if (klineRole == KLineRole::PreDayClose)
	{
		return QString::number(pRichData->preDayClose, 'f', 2);
	}

	if (pRichData->indexRichData)
	{
		if (klineRole == KLineRole::TimeI)
		{
			std::string timestr = CGlobal::GetTickTimeStr(pRichData->indexRichData->time).c_str();
			return CQtGlobal::StdString_QString(timestr);
		}
		if (klineRole == KLineRole::OpenI)
		{
			return QString::number(pRichData->indexRichData->open, 'f', 2);
		}
		if (klineRole == KLineRole::HighI)
		{
			return QString::number(pRichData->indexRichData->high, 'f', 2);
		}
		if (klineRole == KLineRole::LowI)
		{
			return QString::number(pRichData->indexRichData->low, 'f', 2);
		}
		if (klineRole == KLineRole::CloseI)
		{
			return QString::number(pRichData->indexRichData->close, 'f', 2);
		}
		if (klineRole == KLineRole::VolI)
		{
			return QString::number(pRichData->indexRichData->vol);
		}
		if (klineRole == KLineRole::DifI)
		{
			return QString::number(pRichData->indexRichData->dif, 'f', 2);
		}
		if (klineRole == KLineRole::DeaI)
		{
			return QString::number(pRichData->indexRichData->dea, 'f', 2);
		}
		if (klineRole == KLineRole::MacdI)
		{
			return QString::number(pRichData->indexRichData->macd, 'f', 2);
		}
		if (klineRole == KLineRole::DivTypeI)
		{
			std::string str = CTransToStr::Get_DivergenceType(pRichData->indexRichData->divType);
			return CQtGlobal::StdString_QString(str);
		}
		if (klineRole == KLineRole::IsUTurnI)
		{
			return pRichData->indexRichData->isUTurn ? "1" : "0";
		}


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

