//
// Created by fan tang on 2026/11/21.
//

#include "TableModelValueMaker.h"
#include "Factory.h"
#include <Factory_Log.h>
#include <Global.h>

CTableModelValueMaker::CTableModelValueMaker()
{
}

void CTableModelValueMaker::SetCodeId(const std::string &codeId)
{
    m_codeId = codeId;
    return;
}

bool CTableModelValueMaker::Go()
{
    if (!FillClose()) return false;
    if (!FillTableModelValue()) return false;
    return true;
}

ITableModelValue CTableModelValueMaker::GetResult()
{
    return m_tableModelValue;
}

bool CTableModelValueMaker::FillClose()
{
    IQuery query;
    query.byReqType = 0;
    query.dwSubscribeNum = 1;

    IKLines klines;
    MakeAndGet_Env()->GetDB_KLine()->GetKLines(m_codeId, ITimeType::M1, query, klines);
    if (klines.size() != 1)
    {
        Log_Print(LogLevel::Critical, fmt::format("in CTableModelValueMaker::FillClose, klines size = {}", klines.size()));
        return false;
    }
    m_tableModelValue.close = klines[0].close;
    return true;

}

bool CTableModelValueMaker::FillTableModelValue()
{
    if (!FillTableModelValueOneItem(ITimeType::M1, m_tableModelValue.tableModelValue_m1)) return false;
    if (!FillTableModelValueOneItem(ITimeType::M5, m_tableModelValue.tableModelValue_m5)) return false;
    if (!FillTableModelValueOneItem(ITimeType::M15, m_tableModelValue.tableModelValue_m15)) return false;
    if (!FillTableModelValueOneItem(ITimeType::M30, m_tableModelValue.tableModelValue_m30)) return false;
    if (!FillTableModelValueOneItem(ITimeType::H1, m_tableModelValue.tableModelValue_h1)) return false;
    if (!FillTableModelValueOneItem(ITimeType::D1, m_tableModelValue.tableModelValue_d1)) return false;
    return true;
}

bool CTableModelValueMaker::FillTableModelValueOneItem(ITimeType timeType, ITableModelValueOneItem& tableModelValueOneItem)
{
    // 填写ma部分
    IQuery query;
    query.byReqType = 0;
    query.dwSubscribeNum = 1;

    IAvgValues mas;
    MakeAndGet_Env()->GetDB_Ma()->GetValues(m_codeId, timeType, query, mas);
    if (mas.size() != 1)
    {
        Log_Print(LogLevel::Critical, fmt::format("in CTableModelValueMaker::FillTableModelValueOneItem, mas size = {}", mas.size()));
        return false;
    }
    tableModelValueOneItem.v5 = mas[0].v5;
    tableModelValueOneItem.v20 = mas[0].v20;
    tableModelValueOneItem.v60 = mas[0].v60;
    tableModelValueOneItem.v200 = mas[0].v200;

    // 填写macd部分
    IMacdValues macdValues;
    MakeAndGet_Env()->GetDB_Macd()->GetValues(m_codeId, timeType, query, macdValues);
    if (macdValues.size() != 1)
    {
        Log_Print(LogLevel::Critical, fmt::format("in CTableModelValueMaker::FillTableModelValueOneItem, macdValues size = {}", macdValues.size()));
        return false;
    }
    tableModelValueOneItem.macd_diff = macdValues[0].dif;
    tableModelValueOneItem.macd_bar = macdValues[0].macd;

    // 填写divtype部分
    query.byReqType = 0;
    query.dwSubscribeNum = 2;

    IDivTypeValues divTypeValues;
    MakeAndGet_Env()->GetDB_DivType()->GetValues(m_codeId, timeType, query, divTypeValues);
    if (divTypeValues.size() != 2)
    {
        Log_Print(LogLevel::Critical, fmt::format("in CTableModelValueMaker::FillTableModelValueOneItem, divTypeValues size = {}", divTypeValues.size()));
        return false;
    }
    tableModelValueOneItem.lastDivType = divTypeValues[0];
    tableModelValueOneItem.thisDivType = divTypeValues[1];


    return true;

}
