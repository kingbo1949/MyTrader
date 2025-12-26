// #include "pch.h"
#include "MyMath.h"
#include <numeric>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>

double CMath::Accumulate(const std::vector<double>& data)
{
	return std::accumulate(data.begin(), data.end(), 0.0);
}

double CMath::Mean(const std::vector<double>& data)
{
	return Accumulate(data) / data.size();
}

double CMath::stddev(const std::vector<double>& data)
{
	using namespace boost::accumulators;
	accumulator_set<double, stats<tag::variance>> acc;

	for (double x : data) {
		acc(x);
	}

	double variance = boost::accumulators::variance(acc);
	double stddev = std::sqrt(variance);
	return stddev;
}
