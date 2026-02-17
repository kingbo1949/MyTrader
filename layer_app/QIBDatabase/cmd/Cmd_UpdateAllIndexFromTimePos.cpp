

#include "Cmd_UpdateAllIndexFromTimePos.h"
#include <Factory_Log.h>
#include "../src/Factory.h"
#include "../src/Calculator/Calculator.h"


CCmd_UpdateAllIndexFromTimePos::CCmd_UpdateAllIndexFromTimePos(std::string codeId, ITimeType timeType, time_t beginPos)
    :m_codeId(codeId), m_timeType(timeType), m_beginPos(beginPos)
{

}

void CCmd_UpdateAllIndexFromTimePos::operator()()
{
    IKLines klines;
    GetKline_RecountQuery_TimePos(m_codeId, m_timeType, m_beginPos, klines);
    // 更新指标
    Update_Ma(klines);
    Update_Atr(klines);
    Update_Macd(klines);
    Update_DivType(klines);

    Log_Print(LogLevel::Info, fmt::format("{} over", GetThreadName().c_str()));
    return ;
}

std::string CCmd_UpdateAllIndexFromTimePos::GetThreadName()
{
    std::string timetypeStr = Trans_Str(m_timeType);
    std::string str = fmt::format("Cmd_UpdateAllIndexFromTimePos_{}_{}", m_codeId.c_str(), timetypeStr.c_str());
    return str;
}

void CCmd_UpdateAllIndexFromTimePos::Update_Ma(const IKLines& klines)
{
    MakeAndGet_Env()->GetDB_Ma()->RemoveByRange(m_codeId, m_timeType, klines[0].time, LLONG_MAX);

    for (const auto& kline : klines)
    {
        MakenAndGet_Calculator_Ma()->Update(m_codeId, m_timeType, kline);
    }
    return;

}

void CCmd_UpdateAllIndexFromTimePos::Update_Ema(const IKLines &klines)
{
    MakeAndGet_Env()->GetDB_Ema()->RemoveByRange(m_codeId, m_timeType, klines[0].time, LLONG_MAX);

    for (const auto& kline : klines)
    {
        MakenAndGet_Calculator_Ema()->Update(m_codeId, m_timeType, kline);
    }

    return;

}

void CCmd_UpdateAllIndexFromTimePos::Update_VwMa(const IKLines &klines)
{
    MakeAndGet_Env()->GetDB_VwMa()->RemoveByRange(m_codeId, m_timeType, klines[0].time, LLONG_MAX);

    for (const auto& kline : klines)
    {
        MakenAndGet_Calculator_VwMa()->Update(m_codeId, m_timeType, kline);
    }

}

void CCmd_UpdateAllIndexFromTimePos::Update_Atr(const IKLines &klines)
{
    MakeAndGet_Env()->GetDB_Atr()->RemoveByRange(m_codeId, m_timeType, klines[0].time, LLONG_MAX);

    for (const auto& kline : klines)
    {
        MakenAndGet_Calculator_Atr()->Update(m_codeId, m_timeType, kline);
    }

}

void CCmd_UpdateAllIndexFromTimePos::Update_Macd(const IKLines &klines)
{
    MakeAndGet_Env()->GetDB_Macd()->RemoveByRange(m_codeId, m_timeType, klines[0].time, LLONG_MAX);

    for (const auto& kline : klines)
    {
        MakenAndGet_Calculator_Macd()->Update(m_codeId, m_timeType, kline);
    }
    return;

}

void CCmd_UpdateAllIndexFromTimePos::Update_DivType(const IKLines &klines)
{
    MakeAndGet_Env()->GetDB_DivType()->RemoveByRange(m_codeId, m_timeType, klines[0].time, LLONG_MAX);

    for (const auto& kline : klines)
    {
        MakenAndGet_Calculator_DivType()->Update(m_codeId, m_timeType, kline);
    }
    return;

}
