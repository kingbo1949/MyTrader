
#pragma once
#include "ScanKShape.h"

class CScanKShape_BreakMix : public CScanKShape
{
public:
    explicit CScanKShape_BreakMix(QTableView* pTableView);
    ~CScanKShape_BreakMix() override { ; };

protected:
    KLineShapePtrs				MakeKLineShapes() final;


};

