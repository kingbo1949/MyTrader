#pragma once

#include <memory>
#include <base_struc.h>
#include <QtWidgets/QTableView>
#include <qstandarditemmodel.h>
#include "../klineShape/KShapeDefine.h"
#include "../klineShape/MakeScanPacket.h"

// 扫描所有品种的K线形态并显示
class CScanKShape
{
public:
	CScanKShape(QTableView* pTableView);
	virtual ~CScanKShape() { ; };

	void 						Go();

protected:
	QTableView*					m_showTableView;
	QStandardItemModel*			m_model_kshape;
	KShapeOutputs				Scan();
	void						Show(const KShapeOutputs& kShapeOutputs);		

	// 初始化表头
	void						InitTableHeader();

	// 转换记录
	TableViewRecord				TransToTableRecord(const KShapeOutput& kShapeOutput);

	virtual void				Package(MakeScanPacketPtr pMakeScanPacket) = 0;


};
typedef std::shared_ptr<CScanKShape> ScanKShapePtr;

