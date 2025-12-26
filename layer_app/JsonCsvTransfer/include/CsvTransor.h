#pragma once

#include <map>
#include <string>

class CCsvTransor
{
public:
	CCsvTransor() { ; };
	virtual ~CCsvTransor() { ; };

	// 检查是否tital行, 含有keyWord即认为是tital行		
	bool							IsTitleLine(const std::string& lineStr, const std::string& keyWord) ;


protected:
	std::map<std::string, size_t>	m_titleIndex;

	size_t							GetIndex(const std::string& field);

	void							MakeTitleIndex(const std::string& fileName);

};

