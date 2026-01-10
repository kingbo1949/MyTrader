#include "../stdafx.h"
#include "TableModel_Tick_Future.h"
#include <Global.h>
#include <QtGlobal.h>
#include <Factory_IBGlobalShare.h>
CTableModel_Tick_Future::CTableModel_Tick_Future(IbContractPtr pContract)
	:CTableModel_Tick(pContract)
{
	m_headers = { "Time", "Last", "TotalVol", "TurnOver", "Bid5", "BidVol5", "Bid4", "BidVol4", "Bid3", "BidVol3", "Bid2", "BidVol2", "Bid1", "BidVol1", "->",
		"Ask1", "AskVol1", "Ask2", "AskVol2", "Ask3", "AskVol3", "Ask4", "AskVol4", "Ask5", "AskVol5", };
}

QVariant CTableModel_Tick_Future::data(const QModelIndex& index, int role) const
{
	if (role == Qt::BackgroundRole) return data_background(index);
	if (role != Qt::DisplayRole) return QVariant();

	IBTickPtr tick = m_ticks[index.row()];

	TickFutureRole tickRole = static_cast<TickFutureRole>(index.column());
	if (tickRole == TickFutureRole::Time)
	{
		std::string timestr = CGlobal::GetTickTimeStr(tick->time).c_str();
		return CQtGlobal::StdString_QString(timestr);
	}
	if (tickRole == TickFutureRole::Last)
	{
		return QString::number(tick->last * m_pContract->minMove, 'f', 4);
	}
	if (tickRole == TickFutureRole::TotalVol)
	{
		return QString::number(tick->totalVol);
	}
	if (tickRole == TickFutureRole::TurnOver)
	{
		return QString::number(tick->turnOver, 'f', 4);
	}
	if (tickRole == TickFutureRole::Bid1)
	{
		return QString::number(tick->bidAsks[0].bid * m_pContract->minMove, 'f', 2);
	}
	if (tickRole == TickFutureRole::BidVol1)
	{
		return QString::number(tick->bidAsks[0].bidVol);
	}
	if (tickRole == TickFutureRole::Bid2)
	{
		if (tick->bidAsks.size() < 2) return "";
		return QString::number(tick->bidAsks[1].bid * m_pContract->minMove, 'f', 2);
	}
	if (tickRole == TickFutureRole::BidVol2)
	{
		if (tick->bidAsks.size() < 2) return "";
		return QString::number(tick->bidAsks[1].bidVol);
	}
	if (tickRole == TickFutureRole::Bid3)
	{
		if (tick->bidAsks.size() < 3) return "";
		return QString::number(tick->bidAsks[2].bid * m_pContract->minMove, 'f', 2);
	}
	if (tickRole == TickFutureRole::BidVol3)
	{
		if (tick->bidAsks.size() < 3) return "";
		return QString::number(tick->bidAsks[2].bidVol);
	}
	if (tickRole == TickFutureRole::Bid4)
	{
		if (tick->bidAsks.size() < 4) return "";
		return QString::number(tick->bidAsks[3].bid * m_pContract->minMove, 'f', 2);
	}
	if (tickRole == TickFutureRole::BidVol4)
	{
		if (tick->bidAsks.size() < 4) return "";
		return QString::number(tick->bidAsks[3].bidVol);
	}
	if (tickRole == TickFutureRole::Bid5)
	{
		if (tick->bidAsks.size() < 5) return "";
		return QString::number(tick->bidAsks[4].bid * m_pContract->minMove, 'f', 2);
	}
	if (tickRole == TickFutureRole::BidVol5)
	{
		if (tick->bidAsks.size() < 5) return "";
		return QString::number(tick->bidAsks[4].bidVol);
	}
	if (tickRole == TickFutureRole::Arrow)
	{
		return "->";
	}


	if (tickRole == TickFutureRole::Ask1)
	{
		return QString::number(tick->bidAsks[0].ask * m_pContract->minMove, 'f', 2);
	}
	if (tickRole == TickFutureRole::AskVol1)
	{
		return QString::number(tick->bidAsks[0].askVol);
	}
	if (tickRole == TickFutureRole::Ask2)
	{
		if (tick->bidAsks.size() < 2) return "";
		return QString::number(tick->bidAsks[1].ask * m_pContract->minMove, 'f', 2);
	}
	if (tickRole == TickFutureRole::AskVol2)
	{
		if (tick->bidAsks.size() < 2) return "";
		return QString::number(tick->bidAsks[1].askVol);
	}
	if (tickRole == TickFutureRole::Ask3)
	{
		if (tick->bidAsks.size() < 3) return "";
		return QString::number(tick->bidAsks[2].ask * m_pContract->minMove, 'f', 2);
	}
	if (tickRole == TickFutureRole::AskVol3)
	{
		if (tick->bidAsks.size() < 3) return "";
		return QString::number(tick->bidAsks[2].askVol);
	}
	if (tickRole == TickFutureRole::Ask4)
	{
		if (tick->bidAsks.size() < 4) return "";
		return QString::number(tick->bidAsks[3].ask * m_pContract->minMove, 'f', 2);
	}
	if (tickRole == TickFutureRole::AskVol4)
	{
		if (tick->bidAsks.size() < 4) return "";
		return QString::number(tick->bidAsks[3].askVol);
	}
	if (tickRole == TickFutureRole::Ask5)
	{
		if (tick->bidAsks.size() < 5) return "";
		return QString::number(tick->bidAsks[4].ask * m_pContract->minMove, 'f', 2);
	}
	if (tickRole == TickFutureRole::AskVol5)
	{
		if (tick->bidAsks.size() < 5) return "";
		return QString::number(tick->bidAsks[4].askVol);
	}
	return QVariant();
}

std::string CTableModel_Tick_Future::GetTickStr(IBTickPtr thisTick, IbContractPtr contract)
{
	return CTransDataToStr::GetTickDepthStr_Csv(thisTick, contract->minMove);
}

QVariant CTableModel_Tick_Future::data_background(const QModelIndex& index) const
{
	IBTickPtr tick = m_ticks[index.row()];

	TickFutureRole tickRole = static_cast<TickFutureRole>(index.column());
	if (tickRole == TickFutureRole::Arrow)
	{
		return QColor(255, 87, 34); // 橙色背景
	}

	if (index.row() <= 0) return QVariant();

	IBTickPtr pLast = m_ticks[index.row() - 1];
	bool bidJump = false;
	bool askJump = false;
	bool closeChg = false;
	bool volChg = false;

	bidJump = IsJump_Bid(pLast, tick);
	askJump = IsJump_Ask(pLast, tick);

	if (pLast->last != tick->last)
	{
		closeChg = true;
	}
	if (pLast->totalVol != tick->totalVol)
	{
		volChg = true;
	}

	if (tickRole == TickFutureRole::Bid1 && bidJump)
	{
		return QColor(0, 122, 204); 	// 蓝色背景

	}
	if (tickRole == TickFutureRole::Ask1 && askJump)
	{
		return QColor(0, 122, 204);		// 蓝色背景
	}
	if (tickRole == TickFutureRole::Last && closeChg)
	{
		return QColor(150, 150, 150);	// 深灰背景
	}
	if (tickRole == TickFutureRole::TotalVol && volChg)
	{
		return QColor(150, 150, 150);	// 深灰背景
	}

	return QVariant();
}


bool CTableModel_Tick_Future::IsJump_Bid(IBTickPtr lastTick, IBTickPtr thisTick) const
{
	if (!lastTick || !thisTick) return false;

	if (thisTick->bidAsks[0].bid - lastTick->bidAsks[0].bid >= 8) return true;
	return false;
}

bool CTableModel_Tick_Future::IsJump_Ask(IBTickPtr lastTick, IBTickPtr thisTick) const
{
	if (!lastTick || !thisTick) return false;
	if (lastTick->bidAsks[0].ask - thisTick->bidAsks[0].ask >= 8) return true;
	return false;
}
