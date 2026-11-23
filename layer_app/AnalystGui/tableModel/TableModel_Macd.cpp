#include "TableModel_Macd.h"
#include <QtGlobal.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_QDatabase.h>
#include "../src/BireEyeDataGetter.h"
CTableModel_Macd::CTableModel_Macd()
	:CTableModel()
{
	m_headers = { "Time", "Last", "This", "Diff", "Bar" };
}

void CTableModel_Macd::QueryData()
{
	beginResetModel();

	m_tableItems.clear();

	IBBirdEyeViewValues lastOne = MakeAndGet_BireEyeDataGetter()->GetLastOne();
	IBBirdEyeViewValues thisOne = MakeAndGet_BireEyeDataGetter()->GetThisOne();

	FillTableItems(Time_Type::M1, thisOne.value_m1.macd, lastOne.value_m1.divType, thisOne.value_m1.divType);
	FillTableItems(Time_Type::M5, thisOne.value_m5.macd, lastOne.value_m5.divType, thisOne.value_m5.divType);
	FillTableItems(Time_Type::M15, thisOne.value_m15.macd, lastOne.value_m15.divType, thisOne.value_m15.divType);
	FillTableItems(Time_Type::M30, thisOne.value_m30.macd, lastOne.value_m30.divType, thisOne.value_m30.divType);
	FillTableItems(Time_Type::H1, thisOne.value_h1.macd, lastOne.value_h1.divType, thisOne.value_h1.divType);
	FillTableItems(Time_Type::D1, thisOne.value_d1.macd, lastOne.value_d1.divType, thisOne.value_d1.divType);


	

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

void CTableModel_Macd::FillTableItems(Time_Type timetype, IBMacdPtr thisMacd, IBDivTypePtr lastDivType, IBDivTypePtr thisDivType)
{
	TableItem tableItem;
	tableItem.timetype = timetype;
	tableItem.lastDivType = std::make_shared<CIBDivType>();
	tableItem.thisDivType = std::make_shared<CIBDivType>();
	tableItem.lastDivType->divType = lastDivType->divType;
	tableItem.lastDivType->isUTurn = lastDivType->isUTurn;
	tableItem.thisDivType->divType = thisDivType->divType;
	tableItem.thisDivType->isUTurn = thisDivType->isUTurn;
	tableItem.diff = thisMacd->dif;
	tableItem.bar = thisMacd->macd ;
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
