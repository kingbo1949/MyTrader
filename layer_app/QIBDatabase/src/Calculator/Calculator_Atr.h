#pragma once

#include "Calculator.h"
class CCalculator_Atr : public CCalculator
{
public:
    CCalculator_Atr() ;
    virtual ~CCalculator_Atr() { ; };

    virtual void 		Initialize(const std::string& codeId, ITimeType timeType, const IKLines& klines) override final;

    virtual void		Update(const std::string& codeId, ITimeType timeType, const IKLine& newKline) override final;

protected:
    int                 m_circle;
    void		        UpdateValuesToDb(const std::string& codeId, ITimeType timeType, const IAtrValues& values) ;

    // 计算单根ATR
    double              MakeAtr(const IKLine& thisKline, const IKLine& lastKline);
    double              MakeAtr(const IKLine& thisKline);

    // 按传入的时间点返回其前N根ATR(不包括该时间)
    IAtrValues			GetLastAtrs(const std::string& codeId, ITimeType timeType, long long int time, int count);


};




