#pragma once
#include "../tableModel/TableModel.h"
class CTableModel_Macd : public CTableModel
{
public:
	// 顺序必须与m_headers一样, 这样当需要取第n列的数据时, 可以直接用MacdRole(n)来取，增加可读性
	enum class MacdRole
	{
		TimeType = 0,			// k线类型
		LastDiv = 1,			// 上一根的div状态
		ThisDiv = 2,			// 这一根的div状态
		Diff = 3,				// 当前macd的diff值
		Bar = 4					// 当前macd的bar值
	};
	struct TableItem
	{
		Time_Type		timetype;
		IBDivTypePtr	lastDivType;	// 上一根的div状态
		IBDivTypePtr	thisDivType;	// 这一根的div状态
		double			diff;			// 当前macd的diff值
		double 		    bar;			// 当前macd的bar值

	};
	typedef std::vector<TableItem> TableItems;

	explicit  CTableModel_Macd();
	virtual ~CTableModel_Macd() { ; }


	virtual void							QueryData() override final;

	virtual int								rowCount(const QModelIndex& parent = QModelIndex()) const override final;

	virtual QVariant						data(const QModelIndex& index, int role = Qt::DisplayRole) const override final;

protected:
	TableItems								m_tableItems;

	void									QueryData(Time_Type timetype);


	QVariant								Data_background(const QModelIndex& index) const;


};

