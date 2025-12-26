#include "CsvTransor.h"
#include <vector>
#include <Log.h>
#include <Global.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;


bool CCsvTransor::IsTitleLine(const std::string& lineStr, const std::string& keyWord)
{
	if (lineStr.find(keyWord, 0) != std::string::npos) return true;		// 找到keyWord

	return false;
}

size_t CCsvTransor::GetIndex(const std::string& field)
{
	std::map<std::string, size_t>::const_iterator pos = m_titleIndex.find(field);
	if (pos == m_titleIndex.end())
	{
		printf("%s is not valid field \n", field.c_str());
		exit(-1);
	}
	return pos->second;
}

void CCsvTransor::MakeTitleIndex(const std::string& fileName)
{
	m_titleIndex.clear();


	std::string str = CLog::Instance()->ReadFileToString(fileName);
	trim(str);
	std::vector<std::string> strs;
	split(strs, str, is_any_of("\n"), token_compress_off);

	// 第一行即是title行
	std::string title = strs[0];

	strs.clear();
	split(strs, title, is_any_of(","), token_compress_off);
	strs = CGlobal::Trim(strs);
	for (size_t i = 0; i < strs.size(); ++i)
	{
		std::string onestr = strs[i];
		m_titleIndex[onestr] = i;
	}

}
