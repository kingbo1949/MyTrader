
#include "KLineShape_BreakUTurn.h"
#include <Factory_QDatabase.h>
#include <Factory_IBGlobalShare.h>
Cmd_CheckOneCodeId_BreakUTurn::Cmd_CheckOneCodeId_BreakUTurn(IbContractPtr contract, Time_Type timeType, Tick_T endTime, TopOrBottom topOrBottom)
    :m_contract(contract), m_timeType(timeType), m_endTime(endTime), m_topOrBottom(topOrBottom)
{
    // 查询K线需要回溯多少数据
    int backCount = 600;

    QQuery query;
    query.query_type = QQueryType::BEFORE_TIME;
    query.time_ms = endTime + 1;
    query.query_number = backCount;
    IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(m_contract->codeId, m_timeType, query);
    m_pTradeDay = Make_TradeDayObj(klines);
    return ;
}

KShape Cmd_CheckOneCodeId_BreakUTurn::Go(int &pos)
{
    KShape ret = KShape::Normal;

    // 计算m_endTime上一日的高低价
    IBKLinePtrs lastDayKLines = GetLastTradeDayKLines();
    if (lastDayKLines.empty()) return ret;
    HighAndLow lastday_highlow = CHighFrequencyGlobalFunc::MakeHighAndLow(lastDayKLines, false);


    // 取m_endTime交易日的背离形态
    IBKLinePtrs currentDayKLines = GetCurrentTradeDayKLines();
    IBDivTypePtrs divTypeValues = QueryDivTypeValues();
    for (int i = int(divTypeValues.size()) - 1; i >= 0; --i)
    {
        IBKLinePtr kline = currentDayKLines[i];
        pos = int(divTypeValues.size() - 1 - i);

        if (!IsCompatible(m_topOrBottom, divTypeValues[i]->divType))
        {
            // 遇见不兼容的信号，停止
            return ret;
        }

        // 是否uturn
        if (!divTypeValues[i]->isUTurn) continue;

        if (divTypeValues[i]->divType == DivergenceType::Top || divTypeValues[i]->divType == DivergenceType::TopSub)
        {
            if (kline->low > lastday_highlow.high) return KShape::Div;
        }
        if (divTypeValues[i]->divType == DivergenceType::Bottom || divTypeValues[i]->divType == DivergenceType::BottomSub)
        {
            if (kline->low < lastday_highlow.low) return KShape::Div;
        }
    }
    return ret;
}

IBDivTypePtrs Cmd_CheckOneCodeId_BreakUTurn::QueryDivTypeValues()
{
    IBKLinePtrs klines = GetCurrentTradeDayKLines();
    if (klines.empty()) return IBDivTypePtrs();

    QQuery query;
    query.query_type = QQueryType::BETWEEN_TIME;
    query.time_pair.beginPos = klines[0]->time;
    query.time_pair.endPos = klines.back()->time + 1;

    IBDivTypePtrs divTypeValues = MakeAndGet_QDatabase()->GetDivTypes(m_contract->codeId, m_timeType, query);
    return divTypeValues;

}

IBKLinePtrs Cmd_CheckOneCodeId_BreakUTurn::GetCurrentTradeDayKLines()
{
    return m_pTradeDay->GetTradeDayKLines(m_endTime);
}

IBKLinePtrs Cmd_CheckOneCodeId_BreakUTurn::GetLastTradeDayKLines()
{
    IBKLinePtrs ret;
    Tick_T lastTradeDayTime = 0;
    if (m_pTradeDay->GetLastTradeDay(m_endTime, lastTradeDayTime))
    {
        ret = m_pTradeDay->GetTradeDayKLines(lastTradeDayTime);
    }
    return ret;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CKLineShape_BreakUTurn::CKLineShape_BreakUTurn(const KShapeInput &kShapeInput)
    :CKLineShape(kShapeInput)
{
}

KShape CKLineShape_BreakUTurn::CheckOneCodeId(IbContractPtr contract, int &pos)
{
    Cmd_CheckOneCodeId_BreakUTurn cmd(contract, m_kShapeInput.timeType, m_kShapeInput.endTime, m_kShapeInput.topOrBottom);
    return cmd.Go(pos);

}

