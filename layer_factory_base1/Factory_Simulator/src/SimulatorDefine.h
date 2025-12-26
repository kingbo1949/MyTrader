#pragma once

#include <base_trade.h>

// 撮合方式
enum class MakeDealType
{
	OverPrice = 0,				// 过价成交（priceStatus <= -1可成交）
	SeePrice = 1				// 见价成交（priceStatus <= 0可成交）
};



