#pragma once

#include <string>
#include <memory>
class CToStr
{
public:
	CToStr() {};
	virtual ~CToStr() {};

	virtual std::string			to_str() = 0;

};

typedef std::shared_ptr<CToStr> ToStrPtr;