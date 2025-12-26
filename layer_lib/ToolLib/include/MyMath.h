#pragma once

#include <string>
#include <vector>

class CMath
{
public:
	CMath() { ; };
	virtual ~CMath() { ; };

	static double Accumulate(const std::vector<double>& data);
	static double Mean(const std::vector<double>& data);
	static double stddev(const std::vector<double>& data);

};

