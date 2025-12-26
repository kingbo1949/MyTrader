#pragma once

#include <base_struc.h>


enum class QQueryType
{
	ALL_DATA = 0,				// 查询所有数据
	FROM_CURRENT = 1,			// 表示请求最近多少个单位的数据
	BEFORE_TIME = 2,			// 表示请求某个时间以前(包括该时间)query_number个单位的数据
	AFTER_TIME = 3,				// 表示请求某个时间以后query_number个单位的数据
	BETWEEN_TIME				// 表示请求一个时间段的数据
};

struct QQuery
{
	QQueryType				query_type = QQueryType::FROM_CURRENT;
	size_t					query_number = 0;
	time_t					time_ms = 0;        // 毫秒
	TimePair				time_pair;
};

