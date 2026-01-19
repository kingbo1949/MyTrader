#pragma once
#include "KLineShape.h"
#include "TradeDay.h"

// 判断BreakUTurn形态
// 1. 从输入的endtime开始, 其所在交易日必须存在一个背离uturn
// 2. 找出endtime所在交易日的上一交易日的最高最低价格，该uturn的必须是突破了上一交易日的最高最低价格之后的回勾
// 3. endtime所在交易日的uturn可能不止一个，只要其中一个实现了突破了上一交易日的最高最低价格之后的回勾，即可


class Cmd_CheckOneCodeId_BreakUTurn
{
public:
    Cmd_CheckOneCodeId_BreakUTurn(IbContractPtr contract, Time_Type timeType, Tick_T endTime, TopOrBottom topOrBottom);

    virtual  ~Cmd_CheckOneCodeId_BreakUTurn() { ; };

    KShape              Go(int& pos);

protected:
    IbContractPtr       m_contract;
    Time_Type           m_timeType;
    Tick_T              m_endTime;
    TopOrBottom         m_topOrBottom;
    TradeDayPtr         m_pTradeDay;

    // 查询endTime所在日的背离形态
    IBDivTypePtrs       QueryDivTypeValues();

    // 取m_endTime交易日的klines
    IBKLinePtrs			GetCurrentTradeDayKLines();

    // 取m_endTime上一交易日的klines
    IBKLinePtrs			GetLastTradeDayKLines();


};


class CKLineShape_BreakUTurn : public CKLineShape
{
public:
    CKLineShape_BreakUTurn(const KShapeInput& kShapeInput);

    ~CKLineShape_BreakUTurn() override { ; };

protected:
    // 检查单个品种的背离形态 pos是该形态的位置
    KShape				CheckOneCodeId(IbContractPtr contract, int& pos) final;


};


