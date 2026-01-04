#include "TableModel.h"
#include "TableModel_Ma.h"
#include "TableModel_Macd.h"
#include "../src/Factory_AnalystGui.h"
CTableModel::CTableModel()
	: QAbstractTableModel(nullptr)
{
}

void CTableModel::SetCodeId(const CodeStr& codeId)
{
	m_codeId = codeId;
	QueryData();
	return;

}


TableModelQSPtr g_pTableModel_ma = nullptr;
TableModelQSPtr g_pTableModel_macd = nullptr;

TableModelQSPtr	MakeAndGet_TableModel(TableViewType type)
{
	if (type == TableViewType::Ma)
	{
		if (!g_pTableModel_ma)
		{
			g_pTableModel_ma = QSharedPointer<CTableModel_Ma>(new CTableModel_Ma());

		}
		return g_pTableModel_ma;

	}

	if (type == TableViewType::Macd)
	{
		if (!g_pTableModel_macd)
		{
			g_pTableModel_macd = QSharedPointer<CTableModel_Macd>(new CTableModel_Macd());

		}
		return g_pTableModel_macd;

	}

	return nullptr;

}