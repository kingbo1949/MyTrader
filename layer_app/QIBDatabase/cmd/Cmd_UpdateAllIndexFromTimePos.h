#pragma once

#include <memory>
#include <string>
#include <QDatabase.h>
using namespace IBTrader;
using namespace Ice;


class CCmd_UpdateAllIndexFromTimePos
{
public:
    CCmd_UpdateAllIndexFromTimePos(::std::string codeId, ITimeType timeType, time_t beginPos);
    virtual ~CCmd_UpdateAllIndexFromTimePos() { ; };

    void					operator()();

    std::string				GetThreadName();

protected:
    ::std::string           m_codeId;
    ITimeType               m_timeType;
    time_t                  m_beginPos;


    void                    Update_Ma(const IKLines& klines);
    void                    Update_Ema(const IKLines& klines);
    void                    Update_VwMa(const IKLines& klines);
    void                    Update_Atr(const IKLines& klines);
    void                    Update_Macd(const IKLines& klines);
    void                    Update_DivType(const IKLines& klines);


};
typedef std::shared_ptr<CCmd_UpdateAllIndexFromTimePos> Cmd_UpdateAllIndexFromTimePosPtr;



