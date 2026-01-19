#pragma once
#include "ScanKShape.h"
class CScanKShape_Stock : public CScanKShape
{
public:
	CScanKShape_Stock(QTableView* pTableView);
	virtual ~CScanKShape_Stock() { ; };

protected:

	KLineShapePtrs				MakeKLineShapes() final;


};

