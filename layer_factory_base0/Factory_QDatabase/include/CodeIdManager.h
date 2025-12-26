#pragma once

#include <base_struc.h>
class CCodeIdManager
{
public:
	CCodeIdManager() { ; };
	virtual ~CCodeIdManager() { ; };

	// 得到的所有codeId, 对组合品种不做切分
	virtual  IbContractPtrs			GetCodeId_ForAll() = 0;

	// 得到需要订阅行情的codeId, 对组合品种需要切分
	virtual  IbContractPtrs			GetCodeId_ForSubscribeQ() = 0;



};
typedef std::shared_ptr<CCodeIdManager> CodeIdManagerPtr;

