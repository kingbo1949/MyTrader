#pragma once
#include "TableModel.h"
class CTableModel_Kline : public CTableModel
{
	Q_OBJECT

public:
	// 顺序必须与m_headers一样
	enum class KLineRole {
		Time = 0,
		Open = 1,
		High = 2,
		Low = 3,
		Close = 4,
		Vol = 5
	};

	CTableModel_Kline(IbContractPtr pContract);
	virtual ~CTableModel_Kline() { qDebug() << "CTableModel_Kline realse"; }

	virtual void					QueryData(const TimeZoneOfCodeId& timeZone) override;

	virtual void					SaveData() override;

	virtual int						rowCount(const QModelIndex& parent = QModelIndex()) const override;

	virtual QVariant				data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	virtual QModelIndex				nextBidJump(int beginRow) { return index(0, 0); };

	virtual QModelIndex				nextAskJump(int beginRow) { return index(0, 0); };


protected:
	IBKLinePtrs						m_klines;

	virtual std::string				GetFileName(const CodeStr& codeId) override;


};

