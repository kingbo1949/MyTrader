#pragma once
#include <QDatabase.h>
using namespace IBTrader;
using namespace Ice;

class CRichDataMaker
{
public:
    explicit CRichDataMaker(const std::string& codeId, ITimeType timeType, const IKLines& klines);
    ~CRichDataMaker(){;};

    void                            Go();

    IRichValues		                GetResult();

protected:
    std::string                     m_codeId;
    ITimeType	                    m_timeType;
    IKLines		                    m_klines;
    std::map<time_t, IRichValue>    m_mapRick;


    void                            FillMacd();
    void                            FillMa();
    void                            FillDivType();
    void                            FillAtr();
    void                            FillPreDay();

};


