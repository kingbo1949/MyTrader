#pragma once

#include <memory>
#include <string>
class CAnalystResultMonth
{
public:
	CAnalystResultMonth() { ; };
	virtual ~CAnalystResultMonth() { ; };

	virtual void				Go() = 0;
	
	virtual std::string          PrintResult() = 0;

};
typedef std::shared_ptr<CAnalystResultMonth> AnalystResultMonthPtr;

