#pragma once
#include "SeriesNoMaker.h"
#include <atomic>
#include <memory>
class CSeriesNoMaker_Real : public CSeriesNoMaker
{
public:
	CSeriesNoMaker_Real();
	virtual ~CSeriesNoMaker_Real();

	// 测试结果：执行一次需要18纳秒左右，其中通过MakeAndGet_SeriesNoMaker调用需要10纳秒，
	// 如果把MakeAndGet_SeriesNoMaker指针保存再调用核心函数，则减少到8纳秒，如果使用静态库链接，核心函数需要6纳秒
	// 把这个类从虚基类改成直接类之后，速度从18.9纳秒提高到18.7纳秒
	virtual time_t		GetSeriesNo();

	virtual void		SetSeriesNo(time_t seriesNo);

protected:

	std::atomic<time_t>		m_seriesNo;

};

