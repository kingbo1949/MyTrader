#pragma once

#include <qabstractitemmodel.h>
#include <base_struc.h>

enum class TableViewType
{
	Ma = 0,	
	Macd = 1
};

class CTableModel : public QAbstractTableModel
{
public:
	explicit CTableModel();
	virtual ~CTableModel() { ; }

	void							SetCodeId(const CodeStr& codeId);

	virtual void					QueryData() = 0;

	virtual int						rowCount(const QModelIndex& parent = QModelIndex()) const override = 0;

	virtual int						columnCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return m_headers.size();
	}

	virtual QVariant				data(const QModelIndex& index, int role = Qt::DisplayRole) const override = 0;

	virtual QVariant				headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
	{
		if (role == Qt::DisplayRole)
		{
			if (orientation == Qt::Horizontal)
			{
				return m_headers.at(section);
			}
			else
			{
				return section;
			}
		}
		return QVariant();
	}

protected:
	CodeStr							m_codeId;
	QStringList						m_headers;
};
typedef CTableModel* TableModelQPtr;
typedef QSharedPointer<CTableModel> TableModelQSPtr;


TableModelQSPtr		MakeAndGet_TableModel(TableViewType type);