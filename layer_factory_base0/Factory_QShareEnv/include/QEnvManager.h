#pragma once

#include "QEnvOneCode.h"
class CQEnvManager
{
public:
	CQEnvManager() { ; };
	virtual ~CQEnvManager() { ; };

	virtual QEnvOneCodePtr GetEnvOneCode(CodeHashId codeHashId) = 0;

};
typedef std::shared_ptr<CQEnvManager> QEnvManagerPtr;
