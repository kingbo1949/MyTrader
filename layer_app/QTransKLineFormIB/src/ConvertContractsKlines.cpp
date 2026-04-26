
#include "ConvertContractsKlines.h"
#include <Global.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_HashEnv.h>
#include "AppFuncs.h"
CConvertContractsKlines::CConvertContractsKlines(const std::set<CodeStr> &coodeIds)
    :m_codeIds(coodeIds)
{
}

void CConvertContractsKlines::Go()
{
    for (auto code : m_codeIds)
    {
        ConvertOneContractKLines(code);
    }
}

void CConvertContractsKlines::ConvertOneContractKLines(const CodeStr &targetCodeId)
{
    TimePair timePair;
    timePair.beginPos = CGlobal::GetTimeByStr("20240101 00:00:00") * 1000;
    timePair.endPos = Get_CurrentTime()->GetCurrentTime_millisecond();

    CodeHashId codeHash = Get_CodeIdEnv()->Get_CodeId_Hash(targetCodeId.c_str());
    IbContractPtr contract = MakeAndGet_ContractEnv()->GetContract(codeHash);
    if (contract->securityType == SecurityType::STK)
    {
        // 股票需要删除指定时间段的k线和指标
        CAppFuncs::DelToDb(targetCodeId, Time_Type::S15, timePair);
        CAppFuncs::DelToDb(targetCodeId, Time_Type::M1, timePair);
        CAppFuncs::DelToDb(targetCodeId, Time_Type::M5, timePair);
        CAppFuncs::DelToDb(targetCodeId, Time_Type::M15, timePair);
        CAppFuncs::DelToDb(targetCodeId, Time_Type::M30, timePair);
        CAppFuncs::DelToDb(targetCodeId, Time_Type::H1, timePair);
        CAppFuncs::DelToDb(targetCodeId, Time_Type::D1, timePair);
    }

    // 更新K线和指标
    m_klineConverter_nomix.ConvertOneKLineFromIBToDb(targetCodeId, timePair);

    m_klineConverter_nomix.QueryKLineInDb(targetCodeId);

}
