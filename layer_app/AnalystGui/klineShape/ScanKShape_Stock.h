#pragma once
#include "../klineShape/ScanKShape.h"
class CScanKShape_Stock : public CScanKShape
{
public:
	CScanKShape_Stock(QTableView* pTableView);
	virtual ~CScanKShape_Stock() { ; };

protected:

	virtual void				Package(MakeScanPacketPtr pMakeScanPacket) override final;

};

