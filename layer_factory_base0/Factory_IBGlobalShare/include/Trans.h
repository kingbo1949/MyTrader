#pragma once

#ifdef WIN32

#ifdef FACTORY_IBGLOBALSHARE_EXPORTS
#define FACTORY_IBGLOBALSHARE_API __declspec(dllexport)
#else
#define FACTORY_IBGLOBALSHARE_API __declspec(dllimport)
#endif


#else

#define FACTORY_IBGLOBALSHARE_API 

#endif // WIN32

#include <base_struc.h>
// 除字符串有关的各种变换，与字符串有关的转换在CTransToStr类中
class FACTORY_IBGLOBALSHARE_API CTrans
{
public:
	CTrans() { ; };
	virtual ~CTrans() { ; };

	static std::set<CodeStr>					Get_CodeIds(const IbContractPtrs& contracts);


};

