#pragma once
#include "Factory_Log_Export.h"
#include "ToStr.h"
class FACTORY_LOG_API CToStr_Str : public CToStr
{
public:
	CToStr_Str(const std::string& msg)
		:m_msg(msg)
	{

	}
	virtual ~CToStr_Str() {};

	virtual std::string			to_str()
	{
		return m_msg;
	}

protected:
	std::string					m_msg;

};

