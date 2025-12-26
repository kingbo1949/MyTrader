#pragma once

#include <base_struc.h>
class CCodeIdHashEnv
{
public:
	CCodeIdHashEnv() { ; };
	virtual ~CCodeIdHashEnv() { ; };

	virtual const char*		Get_CodeStrByHashId(CodeHashId hashId) = 0;

	virtual CodeHashId		Get_CodeId_Hash(const char* str) = 0;

	virtual RangePair		Get_CodeId_Range() = 0;
};
typedef std::shared_ptr<CCodeIdHashEnv> CodeIdHashEnvPtr;
