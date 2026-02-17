#pragma once

#include <memory>
#include <string>
#include <QDatabase.h>
using namespace IBTrader;
using namespace Ice;

// 全量更新指标
// 指标的重新计算总是从klines[0].time开始，到最新时间
class CCmd_RecountAllIndex
{
public:
    CCmd_RecountAllIndex(::std::string codeId, ITimeType timeType);
    virtual ~CCmd_RecountAllIndex() { ; };

    void					operator()();

    std::string				GetThreadName();

protected:
    ::std::string           m_codeId;
    ITimeType               m_timeType;

    void                    Recount_Ma(const IKLines& klines);
    void                    Recount_Ema(const IKLines& klines);
    void                    Recount_VwMa(const IKLines& klines);
    void                    Recount_Atr(const IKLines& klines);
    void                    Recount_Macd(const IKLines& klines);
    void                    Recount_DivType(const IKLines& klines);


};
typedef std::shared_ptr<CCmd_RecountAllIndex> Cmd_RecountAllIndexPtr;

