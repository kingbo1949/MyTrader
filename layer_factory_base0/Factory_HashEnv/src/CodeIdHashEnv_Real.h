#pragma once
#include "CodeIdHashEnv.h"
class CCodeIdHashEnv_Real : public CCodeIdHashEnv
{
public:
	CCodeIdHashEnv_Real(const std::set<CodeStr>& vaildCodeIs);
	virtual ~CCodeIdHashEnv_Real() { ; };

	virtual const char*		Get_CodeStrByHashId(CodeHashId hashId) override final;

	virtual CodeHashId		Get_CodeId_Hash(const char* str) override final;

	virtual RangePair		Get_CodeId_Range() override final;

protected:
	char					m_valid_codes[CodeHashIdSize][20] = { {0} };
	RangePair				m_codes_range;
};

