#include "MakePrice_UTurn.h"
#include <Factory_UnifyInterface.h>
#include <Factory_IBGlobalShare.h>
#include <Global.h>
#include <Factory_TShareEnv.h>
#include "../KLineAndMaTool.h"
CMakePrice_UTurn::CMakePrice_UTurn(const SubModuleParams &stParams)
    :CMakePrice_Real(stParams)
{
}

bool CMakePrice_UTurn::GetOpenPrice(double &price, RealPriceType &priceType)
{
    priceType = RealPriceType::Original;
    if (Get_Sys_Status() != Thread_Status::Running) return false;

    std::string codeId = m_pStrategyParam->key.targetCodeId;
    Time_Type timetype = m_pStrategyParam->timetype;
    time_t currentTickTime = Get_CurrentTime()->GetCurrentTime_millisecond();

    if (currentTickTime == CGlobal::GetTimeByStr("20250102 14:00:00") * 1000)
    {
        int a = 0;
    }

    IBKLinePtr thisKline = CKLineAndMaTool::GetOneKLine(codeId, timetype, currentTickTime);
    if (!thisKline) return false;

    IBMacdPtr thisMacd = CKLineAndMaTool::GetOneMacd(codeId, timetype, currentTickTime);
    if (!thisMacd) return false;

    IBDivTypePtr thisDivType = CKLineAndMaTool::GetOneDivType(codeId, timetype, currentTickTime);
    if (!thisDivType) return false;

    IBKLinePtr last1D = CKLineAndMaTool::GetLastKline(codeId, Time_Type::D1, currentTickTime);
    if (!last1D) return false;

    if (m_stParams.buyOrSell == BuyOrSell::Buy)
    {
        if (thisKline->close > last1D->low) return false;
        if (thisDivType->isUTurn && thisMacd->dif < -50)
        {
            price = thisKline->close;
            return true;
        }
    }
    else
    {
        if (thisKline->close < last1D->high) return false;
        if (thisDivType->isUTurn && thisMacd->dif > 40)
        {
            price = thisKline->close;
            return true;
        }

    }
    return false;
}

bool CMakePrice_UTurn::GetCoverPrice(double &price, RealPriceType &priceType)
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

    // 得到开仓单成交时间
    time_t lastOpenDealTime = MakeAndGet_DbLastOrder()->GetLastOrderDealTime(OrderKey(m_stParams.strategyIdHashId, StSubModule::SubOpen));
    std::string timestr = CGlobal::GetTickTimeStr(lastOpenDealTime);

    // 开仓信号bar
    IBKLinePtr signalKline = CKLineAndMaTool::GetOneKLine(codeId, timetype, lastOpenDealTime);
    if (!signalKline) return false;

    IBKLinePtr thisKline = CKLineAndMaTool::GetOneKLine(codeId, timetype, currentTickTime);
    if (!thisKline) return false;

    IBMacdPtr thisMacd = CKLineAndMaTool::GetOneMacd(codeId, timetype, currentTickTime);
    if (!thisMacd) return false;

    IBDivTypePtr thisDivType = CKLineAndMaTool::GetOneDivType(codeId, timetype, currentTickTime);
    if (!thisDivType) return false;

    IBKLinePtr last1D = CKLineAndMaTool::GetLastKline(codeId, Time_Type::D1, currentTickTime);
    if (!last1D) return false;

    if (thisKline->time == signalKline->time) return false;


    if (m_stParams.buyOrSell == BuyOrSell::Buy )
    {
        if (thisKline->high > signalKline->high)
        {
            // 止损
            price = signalKline->high;
            return true;
        }

        // 买平仓
        //if (thisKline->close > last1D->low) return false;   // 49 5343 40% 23%
        // if (thisKline->close > last1D->close) return false;   // 111 7060 34% 31%
        if (thisKline->close > last1D->low  ) return false; // 69 6948 37% 31%
        if (thisDivType->isUTurn && thisMacd->dif < -0)
        {
            price = thisKline->close;
            return true;
        }

    }
    if (m_stParams.buyOrSell == BuyOrSell::Sell )
    {
        if (thisKline->low < signalKline->low)
        {
            // 止损
            price = signalKline->low;
            return true;
        }

        // 卖平仓
        if (thisKline->close < last1D->high) return false;
        if (thisDivType->isUTurn && thisMacd->dif > 50)
        {
            price = thisKline->close;
            return true;
        }

    }

    return false;

}

