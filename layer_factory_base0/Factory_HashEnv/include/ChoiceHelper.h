#pragma once

#include "Factory_HashEnv_Export.h"



#include <base_trade.h>
class FACTORY_HASHENV_API CChoiceHelper
{
public:
	CChoiceHelper() { ; };
	virtual ~CChoiceHelper() { ; };

	// 拆分混合品种Id
	static MixCode			GetMixCode(const CodeStr& mixCodeId);

	static BuyOrSell		MakeBuyOrSell_NoMix(StrategyIdHashId strategyIdHashId, RoundAction openOrCover);

	static BuyOrSell		MakeBuyOrSell_Mix(StrategyIdHashId strategyIdHashId, FirstOrSecond firstOrSecond, RoundAction openOrCover);

	static BuyOrSell		MakeBuyOrSell_Mix(StrategyIdHashId strategyIdHashId, CodeHashId codeHash, RoundAction openOrCover);

	// 开仓单导致的方向
	static LongOrShort		MakeLongOrShort_NoMix(StrategyIdHashId strategyIdHashId);

	// 开仓单导致的方向
	static LongOrShort		MakeLongOrShort_Mix(StrategyIdHashId strategyIdHashId, CodeHashId codeHash);

	static FirstOrSecond	MakeFirstOrSecond(StrategyIdHashId strategyIdHashId, CodeHashId codeHash);

	static CodeHashId		GetCodeHash(StrategyIdHashId strategyIdHashId, FirstOrSecond firstOrSecond);


};

