//
// Created by kingb on 2025/12/18.
//
#include "base_trade.h"
#include <spdlog/spdlog.h>

StrategyId Strategykey::ToStr()
{
    if (str == "")
    {
        std::string buyOrSellStr = "";
        if (buyOrSell == BuyOrSell::Buy)
        {
            buyOrSellStr = "Buy";
        }
        else
        {
            buyOrSellStr = "Sell";
        }

        str = fmt::format("{}_{}_{}_{}", strategyName.c_str(), targetCodeId.c_str(), buyOrSellStr.c_str(), id);
    }
    return str;

}
