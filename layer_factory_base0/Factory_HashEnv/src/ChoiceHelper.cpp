#include "pch.h"
#include "ChoiceHelper.h"
#include <Global.h>
#include <Factory_Log.h>
#include "Factory_HashEnv.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
using namespace boost::algorithm;

MixCode CChoiceHelper::GetMixCode(const CodeStr& mixCodeId)
{
	MixCode ret;

	std::vector<std::string> strs;
	boost::algorithm::split(strs, mixCodeId, is_any_of("_"), token_compress_off);
	strs = CGlobal::Trim(strs); 

	if (strs.size() != 2)
	{
		Log_Print(LogLevel::Err, fmt::format("can not split {} to two codeId", mixCodeId.c_str()));
		exit(-1);
	}
	ret.myHashId = Get_CodeIdEnv()->Get_CodeId_Hash(mixCodeId.c_str());

	ret.firstCode = strs[0];
	ret.firstHash = Get_CodeIdEnv()->Get_CodeId_Hash(ret.firstCode.c_str());

	ret.secondCode = strs[1];
	ret.secondHash = Get_CodeIdEnv()->Get_CodeId_Hash(ret.secondCode.c_str());

	return ret;

}

BuyOrSell CChoiceHelper::MakeBuyOrSell_NoMix(StrategyIdHashId strategyIdHashId, RoundAction openOrCover)
{
	StrategyParamPtr pStrategyParam = Get_StrategyParamEnv()->Get_StrategyParam(strategyIdHashId);

	if (pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		if (openOrCover == RoundAction::Open_Round)
		{
			return BuyOrSell::Buy;
		}
		else
		{
			return BuyOrSell::Sell;
		}
	}
	else
	{
		if (openOrCover == RoundAction::Open_Round)
		{
			return BuyOrSell::Sell;
		}
		else
		{
			return BuyOrSell::Buy;
		}

	}
}

BuyOrSell CChoiceHelper::MakeBuyOrSell_Mix(StrategyIdHashId strategyIdHashId, FirstOrSecond firstOrSecond, RoundAction openOrCover)
{
	StrategyParamPtr pStrategyParam = Get_StrategyParamEnv()->Get_StrategyParam(strategyIdHashId);
	if (pStrategyParam->key.buyOrSell == BuyOrSell::Buy)
	{
		if (firstOrSecond == FirstOrSecond::First)
		{
			if (openOrCover == RoundAction::Open_Round)
			{
				return BuyOrSell::Buy;
			}
			else
			{
				return BuyOrSell::Sell;
			}
		}
		else
		{
			if (openOrCover == RoundAction::Open_Round)
			{
				return BuyOrSell::Sell;
			}
			else
			{
				return BuyOrSell::Buy;
			}

		}
	}
	else
	{
		if (firstOrSecond == FirstOrSecond::First)
		{
			if (openOrCover == RoundAction::Open_Round)
			{
				return BuyOrSell::Sell;
			}
			else
			{
				return BuyOrSell::Buy;
			}
		}
		else
		{
			if (openOrCover == RoundAction::Open_Round)
			{
				return BuyOrSell::Buy;
			}
			else
			{
				return BuyOrSell::Sell;
			}

		}
	}

}

BuyOrSell CChoiceHelper::MakeBuyOrSell_Mix(StrategyIdHashId strategyIdHashId, CodeHashId codeHash, RoundAction openOrCover)
{
	FirstOrSecond firstOrSecnd = MakeFirstOrSecond(strategyIdHashId, codeHash);
	return MakeBuyOrSell_Mix(strategyIdHashId, firstOrSecnd, openOrCover);
}

LongOrShort CChoiceHelper::MakeLongOrShort_NoMix(StrategyIdHashId strategyIdHashId)
{
	StrategyParamPtr pStrategyParam = Get_StrategyParamEnv()->Get_StrategyParam(strategyIdHashId);
	if (pStrategyParam->key.buyOrSell == BuyOrSell::Buy) return LongOrShort::LongT;

	return LongOrShort::ShortT;
}

LongOrShort CChoiceHelper::MakeLongOrShort_Mix(StrategyIdHashId strategyIdHashId, CodeHashId codeHash)
{
	BuyOrSell buyOrSell = MakeBuyOrSell_Mix(strategyIdHashId, codeHash, RoundAction::Open_Round);
	if (buyOrSell == BuyOrSell::Buy) return LongOrShort::LongT;
	return LongOrShort::ShortT;

}

FirstOrSecond CChoiceHelper::MakeFirstOrSecond(StrategyIdHashId strategyIdHashId, CodeHashId codeHash)
{
	StrategyParamPtr pStrategyParam = Get_StrategyParamEnv()->Get_StrategyParam(strategyIdHashId);
	MixCode mixcode = GetMixCode(pStrategyParam->key.targetCodeId);
	if (mixcode.firstHash == codeHash) return FirstOrSecond::First;
	if (mixcode.secondHash == codeHash) return FirstOrSecond::Second;

	Log_Print(LogLevel::Err, "invalid codeHash");
	exit(-1);
	return FirstOrSecond::Second;
}

CodeHashId CChoiceHelper::GetCodeHash(StrategyIdHashId strategyIdHashId, FirstOrSecond firstOrSecond)
{
	StrategyParamPtr pStrategyParam = Get_StrategyParamEnv()->Get_StrategyParam(strategyIdHashId);
	MixCode mixcode = GetMixCode(pStrategyParam->key.targetCodeId);
	if (firstOrSecond == FirstOrSecond::First) return mixcode.firstHash;
	return mixcode.secondHash;
}

