#pragma once

#include "Factory_IBGlobalShare_Export.h"

#include <base_struc.h>
// 除字符串有关的各种变换，与字符串有关的转换在CTransToStr类中
class FACTORY_IBGLOBALSHARE_API CTrans
{
public:
	CTrans() { ; };
	virtual ~CTrans() { ; };

	static std::set<CodeStr>					Get_CodeIds(const IbContractPtrs& contracts);


};

