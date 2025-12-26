#include "TableModel_Macd.h"
#include <QtGlobal.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
CTableModel_Macd::CTableModel_Macd()
	:CTableModel()
{
	m_headers = { "Time", "Last", "This", "Diff", "Bar" };
}

void CTableModel_Macd::QueryData()
{
	beginResetModel();

	m_tableItems.clear();

	QueryData(Time_Type::S15);
	QueryData(Time_Type::M1);
	QueryData(Time_Type::M5);
	QueryData(Time_Type::M15);
	QueryData(Time_Type::M30);
	QueryData(Time_Type::H1);
	QueryData(Time_Type::D1);

	

	endResetModel();

	return;

}

int CTableModel_Macd::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return static_cast<int>(m_tableItems.size()) ;
}

QVariant CTableModel_Macd::data(const QModelIndex& index, int role) const
{
	// 设置背景色：每 4 行为一组
	if (role == Qt::BackgroundRole)
	{
		return Data_background(index);
	}


	if (role != Qt::DisplayRole) return QVariant();

	TableItem tableItem = m_tableItems[index.row()];

	MacdRole macdRole = static_cast<MacdRole>(index.column());
	if (macdRole == MacdRole::TimeType)
	{
		return CQtGlobal::StdString_QString(CTransToStr::Get_TimeType(tableItem.timetype).c_str() );

	}
	if (macdRole == MacdRole::LastDiv)
	{
		return CQtGlobal::StdString_QString(CTransToStr::Get_DivergenceType(tableItem.lastDivType->divType).c_str() );
	}
	if (macdRole == MacdRole::ThisDiv)
	{
		return CQtGlobal::StdString_QString(CTransToStr::Get_DivergenceType(tableItem.thisDivType->divType).c_str());
	}
	if (macdRole == MacdRole::Diff)
	{
		return QString::number(tableItem.diff, 'f', 2);
	}
	if (macdRole == MacdRole::Bar)
	{
		return QString::number(tableItem.bar, 'f', 2);
	}

	

	return QVariant();
}

void CTableModel_Macd::QueryData(Time_Type timetype)
{
	QQuery query;
	query.query_type = QQueryType::FROM_CURRENT;
	query.query_number = 2;
	IBMacdPtrs macds = MakeAndGet_QDatabase()->GetMacds(m_codeId, timetype, query);
	if (macds.size() != 2) return;

	query.query_type = QQueryType::FROM_CURRENT;
	query.query_number = 2;
	IBDivTypePtrs divTypes = MakeAndGet_QDatabase()->GetDivTypes(m_codeId, timetype, query);
	if (divTypes.size() != 2) return;


	TableItem tableItem;
	tableItem.timetype = timetype;
	tableItem.lastDivType = divTypes[0];
	tableItem.thisDivType = divTypes[1];
	tableItem.diff = macds[1]->dif;
	tableItem.bar = macds[1]->macd ;

	m_tableItems.push_back(tableItem);
	return;


}

QVariant CTableModel_Macd::Data_background(const QModelIndex& index) const
{
	// 每 4 行一组
	QColor back;

	if (index.row() % 2 == 0)
	{
		back = QColor(240, 240, 240); // 偶数行为灰色
	}
	else
	{
		back = QColor(255, 250, 235); // 奇数行为蛋黄色
	}

	if (index.column() == 1)
	{
		if (m_tableItems[index.row()].lastDivType->divType == DivergenceType::Top || m_tableItems[index.row()].lastDivType->divType == DivergenceType::TopSub)
		{
			back = QColor(255, 192, 203); // 粉红色
			if (m_tableItems[index.row()].lastDivType->isUTurn)
			{
				back = QColor(255, 0, 0); // 红色
			}
			
		}
		if (m_tableItems[index.row()].lastDivType->divType == DivergenceType::Bottom || m_tableItems[index.row()].lastDivType->divType == DivergenceType::BottomSub)
		{
			back = QColor(144, 238, 144); // 淡绿色
			if (m_tableItems[index.row()].lastDivType->isUTurn)
			{
				back = QColor(Qt::darkGreen); // 深绿
				//back = QColor(0, 255, 0); // 绿色
			}
		}
	}
	if (index.column() == 2)
	{
		if (m_tableItems[index.row()].thisDivType->divType == DivergenceType::Top || m_tableItems[index.row()].thisDivType->divType == DivergenceType::TopSub)
		{
			back = QColor(255, 192, 203); // 粉红色
			if (m_tableItems[index.row()].thisDivType->isUTurn)
			{
				back = QColor(255, 0, 0); // 红色
			}

		}
		if (m_tableItems[index.row()].thisDivType->divType == DivergenceType::Bottom || m_tableItems[index.row()].thisDivType->divType == DivergenceType::BottomSub)
		{
			back = QColor(144, 238, 144); // 淡绿色
			if (m_tableItems[index.row()].thisDivType->isUTurn)
			{
				back = QColor(Qt::darkGreen); // 深绿
				//back = QColor(0, 255, 0); // 绿色
			}
		}
	}

	return QBrush(back);
}
