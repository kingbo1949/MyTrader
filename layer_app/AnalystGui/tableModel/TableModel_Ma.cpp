#include "TableModel_Ma.h"
#include <Factory_QDatabase.h>
#include <Factory_IBGlobalShare.h>
#include <QtGlobal.h>
CTableModel_Ma::CTableModel_Ma()
	:CTableModel()
{
	m_headers = { "Time", "Circle", "MaValue", "CloseDiff" };
}


void CTableModel_Ma::QueryData()
{
	beginResetModel();

	m_mapMa.clear();
	m_tableItems.clear();
	m_close = std::numeric_limits<double>::quiet_NaN();

	QueryClose();
	// QueryData(Time_Type::S15);
	QueryData(Time_Type::M1);
	QueryData(Time_Type::M5);
	QueryData(Time_Type::M15);
	QueryData(Time_Type::M30);
	QueryData(Time_Type::H1);
	QueryData(Time_Type::D1);

	m_tableItems = GetTableItems();

	endResetModel();

	return;


}

int CTableModel_Ma::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return static_cast<int>(m_tableItems.size());
	
}

QVariant CTableModel_Ma::data(const QModelIndex& index, int role) const
{
	// 设置背景色：每 4 行为一组
	if (role == Qt::BackgroundRole)
	{
		return Data_background(index);
	}


	if (role != Qt::DisplayRole) return QVariant();

	TableItem tableItem = m_tableItems[index.row()];

	MaRole maRole = static_cast<MaRole>(index.column());
	if (maRole == MaRole::TimeType)
	{
		return CQtGlobal::StdString_QString(tableItem.timeType);

	}
	if (maRole == MaRole::Circle)
	{
		return QString::number(tableItem.circle);
	}
	if (maRole == MaRole::MaValue)
	{
		return QString::number(tableItem.maValue, 'f', 2);
	}
	if (maRole == MaRole::CloseDiff)
	{
		return QString::number(tableItem.closeDiff, 'f', 2);
	}

	return QVariant();
}

void CTableModel_Ma::QueryClose()
{
	m_close = std::numeric_limits<double>::quiet_NaN();
	QQuery query;
	query.query_type = QQueryType::FROM_CURRENT;
	query.query_number = 1;
	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(m_codeId, Time_Type::M1, query);
	if (klines.empty()) return;
	m_close = klines.back()->close;
	return;
}

void CTableModel_Ma::QueryData(Time_Type timetype)
{
	QQuery query;
	query.query_type = QQueryType::FROM_CURRENT;
	query.query_number = 1;
	IBAvgValuePtrs avgValues = MakeAndGet_QDatabase()->GetMas(m_codeId, timetype, query);
	if (avgValues.empty()) return;
	// 填充数据
	for (auto value : avgValues)
	{
		AverageValue mavalue;
		mavalue.v5 = value->v5;
		mavalue.v20 = value->v20;
		mavalue.v60 = value->v60;
		mavalue.v200 = value->v200;
		m_mapMa[timetype] = mavalue;
	}
	return;
}

CTableModel_Ma::TableItems CTableModel_Ma::GetTableItems()
{
	TableItems back;
	for (auto& [timetype, value] : m_mapMa)
	{
		TableItems tem;

		TableItem item;
		item.timeType = CTransToStr::Get_TimeType(timetype);

		item.circle = 5;
		item.maValue = value.v5;
		item.closeDiff = m_close - value.v5;
		tem.push_back(item);

		item.circle = 20;
		item.maValue = value.v20;
		item.closeDiff = m_close - value.v20;
		tem.push_back(item);

		item.circle = 60;
		item.maValue = value.v60;
		item.closeDiff = m_close - value.v60;
		tem.push_back(item);

		item.circle = 200;
		item.maValue = value.v200;
		item.closeDiff = m_close - value.v200;
		tem.push_back(item);

		AssignRanks(tem);
		copy(tem.begin(), tem.end(), back_inserter(back));
	}
	return back;

}

void CTableModel_Ma::AssignRanks(std::vector<TableItem>& items)
{
	// 创建索引数组
	std::vector<size_t> indices(items.size());
	for (size_t i = 0; i < indices.size(); ++i) 
	{
		indices[i] = i;
	}

	// 根据closeDiff对索引数组进行排序
	std::sort(indices.begin(), indices.end(),
		[&items](size_t a, size_t b) {
			return fabs(items[a].closeDiff) < fabs(items[b].closeDiff);
		});

	// 给原始vector的rank赋值
	for (size_t i = 0; i < indices.size(); ++i) 
	{
		items[indices[i]].rank = static_cast<int>(i);
	}
}

QVariant CTableModel_Ma::Data_background(const QModelIndex& index) const
{
	// 每 4 行一组
	QColor back;
	int group = index.row() / 4; 
	if (group % 2 == 0)
	{
		back = QColor(240, 240, 240); // 偶数组（0-3, 8-11）为灰色

	}
	else
	{
		back = QColor(255, 250, 235); // 奇数组（4-7）为蛋黄色

	}

	if (index.column() == 3 && m_tableItems[index.row()].rank == 0)  // closediff 列且rank为0
	{
		if (m_tableItems[index.row()].closeDiff > 0)
		{
			back = QColor(144, 238, 144); // 淡绿色
		}
		else if (m_tableItems[index.row()].closeDiff < 0)
		{
			back = QColor(255, 192, 203); // 粉红色
			
		}
	}

	return QBrush(back);
}
