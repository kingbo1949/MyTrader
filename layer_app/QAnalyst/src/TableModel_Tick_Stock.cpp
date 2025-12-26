#include "stdafx.h"
#include "TableModel_Tick_Stock.h"
#include <Global.h>
#include <QtGlobal.h>
#include <Factory_IBGlobalShare.h>
CTableModel_Tick_Stock::CTableModel_Tick_Stock(IbContractPtr pContract)
	:CTableModel_Tick(pContract)
{
	m_headers = { "Time", "Last", "TotalVol", "TurnOver", "Bid", "BidVol", "->", "Ask", "AskVol" };

}


QVariant CTableModel_Tick_Stock::data(const QModelIndex& index, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();

	IBTickPtr tick = m_ticks[index.row()];

	TickStockRole tickRole = static_cast<TickStockRole>(index.column());
	if (tickRole == TickStockRole::Time)
	{
		std::string timestr = CGlobal::GetTickTimeStr(tick->time).c_str();
		return CQtGlobal::StdString_QString(timestr);
	}
	if (tickRole == TickStockRole::Last)
	{
		return QString::number(tick->last * m_pContract->minMove, 'f', 4);
	}
	if (tickRole == TickStockRole::TotalVol)
	{
		return QString::number(tick->totalVol);
	}
	if (tickRole == TickStockRole::TurnOver)
	{
		return QString::number(tick->turnOver, 'f', 4);
	}
	if (tickRole == TickStockRole::Bid)
	{
		return QString::number(tick->bidAsks[0].bid * m_pContract->minMove, 'f', 2);
	}
	if (tickRole == TickStockRole::BidVol)
	{
		return QString::number(tick->bidAsks[0].bidVol);
	}

	if (tickRole == TickStockRole::Arrow)
	{
		return "->";
	}


	if (tickRole == TickStockRole::Ask)
	{
		return QString::number(tick->bidAsks[0].ask * m_pContract->minMove, 'f', 2);
	}
	if (tickRole == TickStockRole::AskVol)
	{
		return QString::number(tick->bidAsks[0].askVol);
	}

	return QVariant();
}

std::string CTableModel_Tick_Stock::GetTickStr(IBTickPtr thisTick, IbContractPtr contract)
{
	return CTransDataToStr::GetStr_Csv(thisTick, contract->minMove);
}
