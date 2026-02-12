#pragma once
#include "TableModel.h"
class CTableModel_Kline : public CTableModel
{
	Q_OBJECT

public:
	// 顺序必须与m_headers一样
	enum class KLineRole
	{
		Time = 0,
		Open = 1,
		High = 2,
		Low = 3,
		Close = 4,
		Vol = 5,
		Dif = 6,			// CIBMacd中的dif
		Dea = 7,			// CIBMacd中的dea
		Macd = 8,			// CIBMacd中的macd
		DivType = 9,		// CIBDivType中的divType
		IsUTurn = 10,		// CIBDivType中的isUTurn
		Atr = 11,			// CIBAtr中的avgAtr
		Ma5 = 12,			// 5周期均线
		Ma20 = 13,			// 20周期均线
		Ma60 = 14,			// 60周期均线
		Ma200 = 15			// 200周期均线
	};


	CTableModel_Kline(IbContractPtr pContract);
	virtual ~CTableModel_Kline() { qDebug() << "CTableModel_Kline realse"; }

	virtual void					QueryData(const TimeZoneOfCodeId& timeZone) override;

	virtual void					SaveData() override;

	virtual int						rowCount(const QModelIndex& parent = QModelIndex()) const override;

	virtual QVariant				data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	virtual QModelIndex				nextBidJump(int beginRow) override final { return index(0, 0); } ;

	virtual QModelIndex				nextAskJump(int beginRow) override final { return index(0, 0); };


protected:
	KLine4Tables					m_kline4Tables;

	virtual std::string				GetFileName(const CodeStr& codeId) override;

	void							FillKline4Tables(const CodeStr& codeId, Time_Type timeType, const IBKLinePtrs &klines);


};

