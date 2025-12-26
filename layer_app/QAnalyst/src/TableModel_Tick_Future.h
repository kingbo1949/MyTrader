#pragma once
#include "TableModel_Tick.h"
class CTableModel_Tick_Future : public CTableModel_Tick
{
	Q_OBJECT

public:
	// 顺序必须与m_headers一样
	enum class TickFutureRole {
		Time = 0,
		Last = 1,
		TotalVol = 2,
		TurnOver = 3,
		Bid5 = 4,
		BidVol5 = 5,
		Bid4 = 6,
		BidVol4 = 7,
		Bid3 = 8,
		BidVol3 = 9,
		Bid2 = 10,
		BidVol2 = 11,
		Bid1 = 12,
		BidVol1 = 13,
		Arrow = 14,
		Ask1 = 15,
		AskVol1 = 16,
		Ask2 = 17,
		AskVol2 = 18,
		Ask3 = 19,
		AskVol3 = 20,
		Ask4 = 21,
		AskVol4 = 22,
		Ask5 = 23,
		AskVol5 = 24
	};

	CTableModel_Tick_Future(IbContractPtr pContract);
	virtual ~CTableModel_Tick_Future() { qDebug() << "CTableModel_Tick_Future realse"; }

	virtual QVariant				data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

protected:
	virtual std::string				GetTickStr(IBTickPtr thisTick, IbContractPtr contract) override final;

	QVariant						data_background(const QModelIndex& index) const;

	virtual bool					IsJump_Bid(IBTickPtr lastTick, IBTickPtr thisTick) const override final;
	virtual bool					IsJump_Ask(IBTickPtr lastTick, IBTickPtr thisTick) const override final;


};

