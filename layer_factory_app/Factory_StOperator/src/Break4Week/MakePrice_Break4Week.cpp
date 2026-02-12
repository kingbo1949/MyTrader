#include "MakePrice_Break4Week.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_UnifyInterface.h>
#include <Global.h>
#include <Factory_TShareEnv.h>
#include <Factory_HashEnv.h>
#include "../KLineAndMaTool.h"

CMakePrice_Break4Week::CMakePrice_Break4Week(const SubModuleParams &stParams)
:CMakePrice_Real(stParams)
{
}

bool CMakePrice_Break4Week::GetOpenPrice(double &price, RealPriceType &priceType)
{
    priceType = RealPriceType::Original;
    if (Get_Sys_Status() != Thread_Status::Running) return false;

    std::string codeId = m_pStrategyParam->key.targetCodeId;
    Time_Type timetype = m_pStrategyParam->timetype;
    time_t currentTickTime = Get_CurrentTime()->GetCurrentTime_millisecond();

    // 开仓时间改成仅允许08:00 - 16:00
    time_t daytime = CHighFrequencyGlobalFunc::GetDayMillisec();
    time_t beginPos = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("08:00:00", 0);
    time_t endPos = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("14:00:00", 0);

    if (currentTickTime < beginPos) return false;
    if (currentTickTime > endPos) return false;

    // 得到平仓单成交时间
    time_t lastCoverDealTime = MakeAndGet_DbLastOrder()->GetLastOrderDealTime(OrderKey(m_stParams.strategyIdHashId, StSubModule::SubCover));
    // 当日已经平仓过，不可再开仓
    if (CHighFrequencyGlobalFunc::GetDayMillisec(lastCoverDealTime) == daytime) return false;


    if (currentTickTime == CGlobal::GetTimeByStr("20250102 14:00:00") * 1000)
    {
        int a = 0;
    }
    IBAtrPtr thisAtr = CKLineAndMaTool::GetOneAtr(codeId, Time_Type::D1, currentTickTime);
    if (!thisAtr) return false;


    IBKLinePtr thisKline = CKLineAndMaTool::GetOneKLine(codeId, timetype, currentTickTime);
    if (!thisKline) return false;

    // 过去20个周期的k线
    IBKLinePtrs klines20 = CKLineAndMaTool::GetKLines_MoreDay(codeId, timetype, currentTickTime,20);
    if (klines20.empty()) return false;

    HighAndLow highLow = CHighFrequencyGlobalFunc::MakeHighAndLow(klines20, false);

    IBKLinePtr last1D = CKLineAndMaTool::GetLastKline(codeId, Time_Type::D1, currentTickTime);
    if (!last1D) return false;

    IBKLinePtrs lastdayKLines = CKLineAndMaTool::GetLastDayKLines(codeId, timetype, currentTickTime);
    if (lastdayKLines.empty()) return false;
    HighAndLow lastdayHighlow = CHighFrequencyGlobalFunc::MakeHighAndLow(lastdayKLines, false);


    if (m_stParams.buyOrSell == BuyOrSell::Buy)
    {
        //if (thisKline->high > highLow.high && highLow.high <  last1D->close)
        if (thisKline->high > highLow.high && highLow.low < lastdayHighlow.low)
        {
            price = highLow.high;
            return true;
        }
    }
    else
    {
        // if (thisKline->low < highLow.low && highLow.high > last1D->high)
        if (thisKline->low < highLow.low && highLow.high > lastdayHighlow.high)
        {
            price = highLow.low;
            return true;
        }

    }
    return false;

}

bool CMakePrice_Break4Week::GetCoverPrice(double &price, RealPriceType &priceType)
{
    priceType = RealPriceType::Original;

    std::string codeId = m_pStrategyParam->key.targetCodeId;
    Time_Type timetype = m_pStrategyParam->timetype;
    time_t currentTickTime = Get_CurrentTime()->GetCurrentTime_millisecond();
    std::string nowstr = CGlobal::GetTickTimeStr(currentTickTime);

    if (currentTickTime == CGlobal::GetTimeByStr("20250107 10:30:00") * 1000)
    {
        int a = 0;
    }

    IBKLinePtr thisKline = CKLineAndMaTool::GetOneKLine(codeId, timetype, currentTickTime);
    if (!thisKline) return false;

    // 过去20个周期的k线
    IBKLinePtrs klines20 = CKLineAndMaTool::GetKLines_MoreDay(codeId, timetype, currentTickTime,20);
    if (klines20.empty()) return false;

    HighAndLow highLow = CHighFrequencyGlobalFunc::MakeHighAndLow(klines20, false);

    // IBKLinePtr last1D = CKLineAndMaTool::GetLastKline(codeId, Time_Type::D1, currentTickTime);
    // if (!last1D) return false;

    StrategyIdHashId strategyIdHashId = Get_StrategyParamEnv()->Get_StrategyParam_Hash(m_pStrategyParam->key.ToStr());
    OrderKey orderkey(strategyIdHashId, StSubModule::SubOpen);
    LocalOrderNo localOrderNo = MakeAndGet_DbLastOrder()->GetLastOrderNo(orderkey);
    OneOrderPtr porder = MakeAndGet_DbInactiveOrder()->GetOne(localOrderNo);
    if (!porder) return false;

    time_t daytime = CHighFrequencyGlobalFunc::GetDayMillisec();
    time_t beginPos = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("08:00:00", 0);
    time_t endPos = daytime + CHighFrequencyGlobalFunc::MakeMilliSecondPart("14:00:00", 0);

    if (thisKline->low > porder->orderPriceReal)
    {
        if (currentTickTime < beginPos) return false;
        if (currentTickTime > endPos) return false;

    }




    if (m_stParams.buyOrSell == BuyOrSell::Buy)
    {
        if (thisKline->high > highLow.high)
        {
            price = highLow.high;
            return true;
        }
    }
    else
    {
        if (thisKline->close < highLow.low )
        {
            price = thisKline->close;
            return true;
        }

    }
    return false;

}
