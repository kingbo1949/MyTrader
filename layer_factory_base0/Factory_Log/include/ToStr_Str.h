#pragma once
#include "ToStr.h"
class CToStr_Str : public CToStr
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

