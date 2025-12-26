#include "pch.h"
#include "HighLowPointOutside.h"
#include <fstream>
#include <iostream>
#include <Global.h>

void CHighLowPointOutside::Init()
{
	std::string filename = GetHighLowPointFileName();

	std::ifstream file(filename);

	if (!file.is_open()) {
		std::cerr << "Error: Could not open the file " << filename << std::endl;
		exit(-1);
	}

	std::string line = "";

	// 读取每一行
	while (std::getline(file, line))
	{
		Tick_T dayMs = 0;
		HighLowPoint highLowPoint;
		if (!TransToHighLowPoint(line, dayMs, highLowPoint)) continue;

		m_dayms_highLowPoint[dayMs] = highLowPoint;
	}
	file.close();
	return;

}

bool CHighLowPointOutside::GetHighLowPoint(Tick_T dayMs, HighLowPoint& highLowPoint)
{
	std::map<Tick_T, HighLowPoint>::const_iterator pos = m_dayms_highLowPoint.find(dayMs);
	if (pos == m_dayms_highLowPoint.end()) return false;

	highLowPoint = pos->second;
	return true;
}

bool CHighLowPointOutside::CanReadHighLowPointFromFile()
{
	return false;
}

std::string CHighLowPointOutside::GetHighLowPointFileName()
{
	return  "./tradepoint/point.csv";
}

bool CHighLowPointOutside::TransToHighLowPoint(const std::string& line, Tick_T& dayMs, HighLowPoint& highLowPoint)
{
	if (line.find("open") != std::string::npos)
	{
		// 这是标题行
		return false;
	}
	std::vector<std::string> strings = CGlobal::SplitStr(line, ",");
	if (strings.size() < 6)
	{
		// 空行或者错误的行
		return false;
	}

	highLowPoint.highPoint = std::stod(strings[3]);
	highLowPoint.lowPoint = std::stod(strings[5]);


	std::string daystr = strings[0];
	daystr.erase(std::remove(daystr.begin(), daystr.end(), '-'), daystr.end());		// 从daystr中删除所有的'-'字符
	daystr += " 00:00:00";
	Second_T dayTime = CGlobal::GetTimeByStr(daystr);
	dayMs = dayTime * 1000;
	return true;
}
