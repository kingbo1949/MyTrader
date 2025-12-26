#include "pch.h"
#include "CodeIdHashEnv_Real.h"

CCodeIdHashEnv_Real::CCodeIdHashEnv_Real(const std::set<CodeStr>& vaildCodeIs)
{
	m_codes_range.beginPos = 1;
	CodeHashId i = 1;

	for (auto onecode : vaildCodeIs)
	{
		m_codes_range.endPos = i;
		strcpy(m_valid_codes[i], onecode.c_str());
		++i;
	}
	return;

}

const char* CCodeIdHashEnv_Real::Get_CodeStrByHashId(CodeHashId hashId)
{
	return *(m_valid_codes + hashId);
}

CodeHashId CCodeIdHashEnv_Real::Get_CodeId_Hash(const char* str)
{
	if (m_codes_range.beginPos == 0)
	{
		printf("CodeHash is not init \n");
		exit(-1);
	}

	for (CodeHashId i = 1; i < CodeHashIdSize; ++i)
	{
		if (strcmp(*(m_valid_codes + i), str) == 0) return i;		// 
		if (strcmp(*(m_valid_codes + i), "") == 0) 		// 
		{
			printf("Get_CodeId_Hash, can not find %s \n", str);
			//exit(-1);
			return 0;
		}

	}
	printf("Get_CodeId_Hash error \n");
	exit(-1);
	return 0;

}

RangePair CCodeIdHashEnv_Real::Get_CodeId_Range()
{
	return m_codes_range;
}
