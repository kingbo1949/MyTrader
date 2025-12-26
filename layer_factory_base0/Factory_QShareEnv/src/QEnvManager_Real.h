#pragma once
#include "QEnvManager.h"
class CQEnvManager_Real : public CQEnvManager
{
public:
	CQEnvManager_Real(IbContractPtrs contracts);
	virtual ~CQEnvManager_Real() { ; };

	virtual QEnvOneCodePtr GetEnvOneCode(CodeHashId codeHashId) override final;


protected:
	// m_oneCodeEnvs在构造函数中初始化好了之后就不再变了，不需要锁定
	QEnvOneCodePtr			m_oneCodeEnvs[CodeHashIdSize];

};

