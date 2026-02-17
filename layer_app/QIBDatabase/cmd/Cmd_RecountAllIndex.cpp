//
// Created by kingb on 2026/2/16.
//

#include "Cmd_RecountAllIndex.h"
#include <Factory_Log.h>
#include <Global.h>
#include "../src/Factory.h"
#include "../src/Calculator/Calculator.h"

CCmd_RecountAllIndex::CCmd_RecountAllIndex(std::string codeId, ITimeType timeType)
    :m_codeId(codeId), m_timeType(timeType)
{

}

void CCmd_RecountAllIndex::operator()()
{
    time_t beginPos = benchmark_milliseconds();
    // 取全部K线
    IKLines klines;
    GetKline_RecountQuery_TimePos(m_codeId, m_timeType, 0, klines);

    // 更新指标
    Recount_Ma(klines);
    Recount_Atr(klines);
    Recount_Macd(klines);
    Recount_DivType(klines);

    time_t endPos = benchmark_milliseconds();

    LogLevel level = LogLevel::Info;
    if (m_timeType == ITimeType::M1)
    {
        level = LogLevel::Warn;
    }
    Log_Print(level, fmt::format("{} over, used = {}", GetThreadName().c_str(), endPos - beginPos));
    return ;
}

std::string CCmd_RecountAllIndex::GetThreadName()
{
    std::string timetypeStr = Trans_Str(m_timeType);
    std::string str = fmt::format("Cmd_RecountAllIndex_{}_{}", m_codeId.c_str(), timetypeStr.c_str());
    return str;
}

void CCmd_RecountAllIndex::Recount_Ma(const IKLines& klines)
{
    MakeAndGet_Env()->GetDB_Ma()->RemoveByRange(m_codeId, m_timeType, 0, LLONG_MAX);
    MakenAndGet_Calculator_Ma()->Initialize(m_codeId, m_timeType, klines);

    return;

}

void CCmd_RecountAllIndex::Recount_Ema(const IKLines &klines)
{
    MakeAndGet_Env()->GetDB_Ema()->RemoveByRange(m_codeId, m_timeType, 0, LLONG_MAX);
    MakenAndGet_Calculator_Ema()->Initialize(m_codeId, m_timeType, klines);
}

void CCmd_RecountAllIndex::Recount_VwMa(const IKLines &klines)
{
    MakeAndGet_Env()->GetDB_VwMa()->RemoveByRange(m_codeId, m_timeType, 0, LLONG_MAX);
    MakenAndGet_Calculator_VwMa()->Initialize(m_codeId, m_timeType, klines);
}

void CCmd_RecountAllIndex::Recount_Atr(const IKLines &klines)
{
    MakeAndGet_Env()->GetDB_Atr()->RemoveByRange(m_codeId, m_timeType, 0, LLONG_MAX);
    MakenAndGet_Calculator_Atr()->Initialize(m_codeId, m_timeType, klines);

}

void CCmd_RecountAllIndex::Recount_Macd(const IKLines &klines)
{
    MakeAndGet_Env()->GetDB_Macd()->RemoveByRange(m_codeId, m_timeType, 0, LLONG_MAX);
    MakenAndGet_Calculator_Macd()->Initialize(m_codeId, m_timeType, klines);

}

void CCmd_RecountAllIndex::Recount_DivType(const IKLines &klines)
{
    MakeAndGet_Env()->GetDB_DivType()->RemoveByRange(m_codeId, m_timeType, 0, LLONG_MAX);
    MakenAndGet_Calculator_DivType()->Initialize(m_codeId, m_timeType, klines);

}
