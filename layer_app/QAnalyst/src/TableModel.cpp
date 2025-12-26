#include "stdafx.h"
#include "TableModel.h"
#include "TableModel_Kline.h"
#include "TableModel_Tick_Future.h"
#include "TableModel_Tick_Stock.h"
#include "Factory_QDatabase.h"
CTableModel::CTableModel(IbContractPtr pContract)
	: QAbstractTableModel(nullptr), m_pContract(pContract)
{
}
int CTableModel::columnCount(const QModelIndex& parent) const
{
	return m_headers.count();
}

QVariant CTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal)
		{
			return m_headers.at(section);
		}
	}
	return QVariant();
}

QModelIndex CTableModel::findTime(const QString& timeStr)
{
	QModelIndex ret;
	for (int row = 0; row < rowCount(); ++row)
	{
		const int TIME_COL = 0; // TIME字段所在的列
		if (data(index(row, TIME_COL)).toString().contains(timeStr, Qt::CaseSensitive))
		{
			return index(row, TIME_COL);
			//tableView->setCurrentIndex(index);
			//tableView->scrollTo(index);
			//qDebug() << "Found at row:" << row << "column:" << col;
			//return;
		}
	}
	return ret;
}


TableModelQSPtr MakeAndGet_TableModel(LineType lineType, const CodeStr& codeId)
{
	IbContractPtr pContract = MakeAndGet_ContractEnv()->GetContract(codeId);
	if (lineType == LineType::UseKLine)
	{
		return QSharedPointer<CTableModel_Kline>(new CTableModel_Kline(pContract));
	}
	if (lineType == LineType::UseTick)
	{

		if (pContract->securityType == SecurityType::STK)
		{
			return QSharedPointer<CTableModel_Tick_Stock>(new CTableModel_Tick_Stock(pContract));
		}
		else
		{
			return QSharedPointer<CTableModel_Tick_Future>(new CTableModel_Tick_Future(pContract));
		}
	}
	return nullptr;
}
