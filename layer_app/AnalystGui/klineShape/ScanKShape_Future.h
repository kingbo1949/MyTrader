#pragma once
#include "ScanKShape.h"
class CScanKShape_Future : public CScanKShape
{
public:
	CScanKShape_Future(QTableView* pTableView);
	virtual ~CScanKShape_Future() { ; };


protected:

	KLineShapePtrs				MakeKLineShapes() final;

};

