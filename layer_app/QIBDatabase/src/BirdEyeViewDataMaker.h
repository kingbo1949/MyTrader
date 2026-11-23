//
// Created by fan tang on 2026/11/22.
//

#pragma once
#include <QDatabase.h>
using namespace IBTrader;
using namespace Ice;


class CBirdEyeViewDataMaker
{

public:
    explicit CBirdEyeViewDataMaker(){;};
    ~CBirdEyeViewDataMaker(){;};

    bool                            Go(const std::string& codeId);

    IBirdEyeViewPair		        GetResult();

protected:
    std::string                     m_codeId;
    IBirdEyeViewPair                m_birdEyeViewData;

    bool                            QueryKLine();
    bool                            QueryKLine(ITimeType timeType, IKLine& lastOne, IKLine& thisOne);

    bool                            QueryMa();
    bool                            QueryMa(ITimeType timeType, IAvgValue& lastOne, IAvgValue& thisOne);

    bool                            QueryMacd();
    bool                            QueryMacd(ITimeType timeType, IMacdValue& lastOne, IMacdValue& thisOne);

    bool                            QueryDivType();
    bool                            QueryDivType(ITimeType timeType, IDivTypeValue& lastOne, IDivTypeValue& thisOne);
};

