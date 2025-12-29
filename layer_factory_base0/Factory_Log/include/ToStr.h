#pragma once
#include "Factory_Log_Export.h"
#include <string>
#include <memory>
class FACTORY_LOG_API CToStr
{
public:
	CToStr() {};
	virtual ~CToStr() {};

	virtual std::string			to_str() = 0;

};

typedef std::shared_ptr<CToStr> ToStrPtr;