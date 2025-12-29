#pragma once
#include "Factory_Log_Export.h"
#include "ToStr.h"
#include <functional>
class FACTORY_LOG_API CToStr_FunctionObj : public CToStr
{
public:
	CToStr_FunctionObj(std::function<std::string()> fr);
	virtual ~CToStr_FunctionObj();

	virtual std::string			to_str() override final;

protected:
	std::function<std::string()> m_fr ;

};

