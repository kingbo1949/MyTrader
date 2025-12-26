#pragma once
#include "../tableModel/TableModel.h"
class CTableModel_Ma : public CTableModel
{
public:
	// 顺序必须与m_headers一样, 这样当需要取第n列的数据时, 可以直接用MaRole(n)来取，增加可读性
	enum class MaRole 
	{
		TimeType = 0,			// k线类型
		Circle = 1,				// 均线周期
		MaValue = 2,			// 均线值
		CloseDiff = 3			// 收盘价与均线差值
	};
	struct TableItem
	{
		std::string		timeType;
		int				circle;
		double			maValue;
		double			closeDiff;
		int				rank;		// 排名 比较一个组中closeDiff的大小, 最小为0，最大为size-1
	};
	typedef std::vector<TableItem> TableItems;

	explicit  CTableModel_Ma();
	virtual ~CTableModel_Ma() { ; }

	virtual void							SetMouseTime(Time_Type timetype, Tick_T mouseTime) { ; };

	virtual void							QueryData() override final;

	virtual int								rowCount(const QModelIndex& parent = QModelIndex()) const override final;

	virtual QVariant						data(const QModelIndex& index, int role = Qt::DisplayRole) const override final;

protected:
	std::map<Time_Type, AverageValue>		m_mapMa;
	double									m_close;
	TableItems								m_tableItems;

	virtual void							QueryClose();
	virtual void							QueryData(Time_Type timetype);

	// 制作显示在表内的数据，排序等特殊处理都在这里
	TableItems								GetTableItems();

	// 排序 为vector中的每个元素赋值rank
	void									AssignRanks(std::vector<TableItem>& items);

	QVariant								Data_background(const QModelIndex& index) const;

};

