#pragma once
#include "../klineShape/ScanKShape.h"
class CScanKShape_Future : public CScanKShape
{
public:
	CScanKShape_Future(QTableView* pTableView);
	virtual ~CScanKShape_Future() { ; };


protected:

	virtual void				Package(MakeScanPacketPtr pMakeScanPacket) override final;

};

