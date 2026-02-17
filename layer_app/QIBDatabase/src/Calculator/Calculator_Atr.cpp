//
// Created by kingb on 2026/1/2.
//

#include "Calculator_Atr.h"
#include "../Factory.h"
#include <algorithm>
#include <numeric>

CCalculator_Atr::CCalculator_Atr()
    :m_circle(14)
{
}

void CCalculator_Atr::Initialize(const std::string &codeId, ITimeType timeType, const IKLines &klines)
{
    IAtrValues ret(klines.size());
    for (auto i = 0; i < klines.size(); ++i)
    {
        ret[i].time = klines[i].time;
    }

    auto sumAtr = [](double acc, const IAtrValue& atrValue)
    {
        return acc + atrValue.thisAtr;
    };

    for (int i = 0; i < klines.size(); ++i)
    {
        // 填写本bar的atr
        if (i == 0)
        {
            ret[i].thisAtr = MakeAtr(klines[i]);
        }else
        {
            ret[i].thisAtr = MakeAtr(klines[i], klines[i - 1]);
        }
        if (i < m_circle - 1)
        {
            // 数据不足，不能计算均值
            ret[i].avgAtr = std::numeric_limits<double>::quiet_NaN();
        }
        else
        {
            // 计算当前周期内的数据总和
            double sum = std::accumulate(ret.begin() + (i - m_circle + 1), ret.begin() + i + 1, 0.0, sumAtr);
            // 计算平均值
            ret[i].avgAtr = sum / m_circle;

        }
    }


    // 更新数据库
    UpdateValuesToDb(codeId, timeType, ret);
    return;

}

void CCalculator_Atr::Update(const std::string &codeId, ITimeType timeType, const IKLine &newKline)
{
    if (!Exist(codeId, timeType, newKline)) return;

    IKLine lastBar;
    if (!GetLastBar(codeId, timeType, newKline, lastBar))
    {
        printf("%s do not have last bar \n", codeId.c_str());
        return ;
    }

    // 计算仅依赖于最近circle个BAR
    IAtrValue value;
    value.time = newKline.time;
    value.thisAtr = MakeAtr(newKline, lastBar);

    // 取前面13根
    IAtrValues values = GetLastAtrs(codeId, timeType, newKline.time, m_circle - 1);
    // 加上当前根
    values.push_back(value);

    auto sumAtr = [](double acc, const IAtrValue& atrValue)
    {
        return acc + atrValue.thisAtr;
    };
    double sum = std::accumulate(values.begin() , values.end() , 0.0, sumAtr);
    // 计算平均值
    value.avgAtr = sum / m_circle;

    // 写入数据库
    MakeAndGet_Env()->GetDB_Atr()->AddOne(codeId, timeType, value);

    return;

}

void CCalculator_Atr::UpdateValuesToDb(const std::string &codeId, ITimeType timeType, const IAtrValues& values)
{
    MakeAndGet_Env()->GetDB_Atr()->AddSome(codeId, timeType, values);
    return;

}

double CCalculator_Atr::MakeAtr(const IKLine &thisKline, const IKLine &lastKline)
{
    double highlow = thisKline.high - thisKline.low;
    double highjump = abs(thisKline.high - lastKline.close);
    double lowjump = abs(thisKline.low - lastKline.close);

    return std::max(highlow, std::max(highjump, lowjump)) ;

}

double CCalculator_Atr::MakeAtr(const IKLine &thisKline)
{
    return thisKline.high - thisKline.low;
}

IAtrValues CCalculator_Atr::GetLastAtrs(const std::string &codeId, ITimeType timeType, long long int time, int count)
{
    IAtrValues back;

    IQuery query;
    query.byReqType = 2;
    query.dwSubscribeNum = count;
    query.tTime = time - 1;

    MakeAndGet_Env()->GetDB_Atr()->GetValues(codeId, timeType, query, back);
    return back;

}

