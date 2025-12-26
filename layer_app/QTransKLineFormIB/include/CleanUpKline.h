#pragma once

#include <Factory_QDatabase.h>
class CCleanUpKline
{
public:
	CCleanUpKline() { ; };
	virtual ~CCleanUpKline() { ; };

	void	Go();

protected:
	void	ClenaUpKLine(const CodeStr& codeId);
	void	ClenaUpKLine(const CodeStr& codeId, Time_Type timetype);

};

