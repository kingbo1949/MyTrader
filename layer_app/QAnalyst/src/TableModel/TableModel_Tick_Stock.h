#pragma once
#include "TableModel_Tick.h"
class CTableModel_Tick_Stock : public CTableModel_Tick
{
	Q_OBJECT

public:

	// 顺序必须与m_headers一样
	enum class TickStockRole {
		Time = 0,
		Last = 1,
		TotalVol = 2,
		TurnOver = 3,
		Bid = 4,
		BidVol = 5,
		Arrow = 6,
		Ask = 7,
		AskVol = 8
	};

	CTableModel_Tick_Stock(IbContractPtr pContract);
	virtual ~CTableModel_Tick_Stock() { qDebug() << "CTableModel_Tick_Stock realse"; ; }



	virtual QVariant				data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

protected:
	virtual std::string				GetTickStr(IBTickPtr thisTick, IbContractPtr contract) override final;



};

