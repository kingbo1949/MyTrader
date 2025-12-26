#pragma once

#include <base_struc.h>
#include <base_trade.h>

struct HighLowPoint
{
	double		highPoint = 0.0;
	double		lowPoint = 0.0;
};

// 从外部文件读取高低点
class CHighLowPointOutside
{
public:
	CHighLowPointOutside() { ; };
	virtual ~CHighLowPointOutside() { ; };


	void										Init();

	// 按日期取高地价
	bool										GetHighLowPoint(Tick_T dayMs, HighLowPoint& highLowPoint);

	// 是否可以使用从文件中读取高低点
	bool										CanReadHighLowPointFromFile();


protected:
	std::string									GetHighLowPointFileName();

	std::map<Tick_T, HighLowPoint>				m_dayms_highLowPoint;		// 日的零点到高低价映射

	bool										TransToHighLowPoint(const std::string& line, Tick_T& dayMs, HighLowPoint& highLowPoint);




};
typedef std::shared_ptr<CHighLowPointOutside> HighLowPointOutsidePtr;

