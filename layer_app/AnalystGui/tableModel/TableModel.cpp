#include "TableModel.h"
#include "TableModel_Ma.h"
#include "TableModel_Ma_Mouse.h"
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


TableModelQSPtr g_pTableModel_ma_nomouse = nullptr;
TableModelQSPtr g_pTableModel_ma_mouse = nullptr;
TableModelQSPtr g_pTableModel_macd = nullptr;

TableModelQSPtr	MakeAndGet_TableModel(TableViewType type)
{
	if (type == TableViewType::Ma)
	{
		if (Get_TableViewMouse() == TableViewMouse::NoMouse)
		{
			if (!g_pTableModel_ma_nomouse)
			{
				g_pTableModel_ma_nomouse = QSharedPointer<CTableModel_Ma>(new CTableModel_Ma());

			}
			return g_pTableModel_ma_nomouse;

		}
		else
		{
			if (!g_pTableModel_ma_mouse)
			{
				g_pTableModel_ma_mouse = QSharedPointer<CTableModel_Ma_Mouse>(new CTableModel_Ma_Mouse());

			}
			return g_pTableModel_ma_mouse;

		}


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