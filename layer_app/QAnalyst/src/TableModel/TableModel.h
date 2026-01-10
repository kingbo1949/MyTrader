#pragma once
#include "../stdafx.h"
#include <qabstractitemmodel.h>
class CTableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	CTableModel(IbContractPtr pContract);
	virtual ~CTableModel() { ; }

	virtual void					QueryData(const TimeZoneOfCodeId& timeZone) = 0;

	virtual void					SaveData() = 0;

	virtual int						rowCount(const QModelIndex& parent = QModelIndex()) const override = 0;

	virtual int						columnCount(const QModelIndex& parent = QModelIndex()) const override;

	virtual QVariant				data(const QModelIndex& index, int role = Qt::DisplayRole) const override = 0;

	virtual QVariant				headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	QModelIndex						findTime(const QString& timeStr);

	virtual QModelIndex				nextBidJump(int beginRow) = 0;

	virtual QModelIndex				nextAskJump(int beginRow) = 0;


protected:
	IbContractPtr					m_pContract;
	QStringList						m_headers;
	TimeZoneOfCodeId				m_timeZone;

	virtual std::string				GetFileName(const CodeStr& codeId) = 0;

};
typedef CTableModel* TableModelQPtr;
typedef QSharedPointer<CTableModel> TableModelQSPtr;

TableModelQSPtr						MakeAndGet_TableModel(LineType lineType, const CodeStr& codeId);

