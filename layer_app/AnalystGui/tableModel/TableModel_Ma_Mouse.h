#pragma once
#include "TableModel_Ma.h"
class CTableModel_Ma_Mouse : public CTableModel_Ma
{
public:

	explicit  CTableModel_Ma_Mouse();
	virtual ~CTableModel_Ma_Mouse() { ; }

	virtual void							SetMouseTime(Time_Type timetype, Tick_T mouseTime) override final;


protected:
	Time_Type								m_mouseTimetype;		// 鼠标时间类型
	Tick_T									m_mouseTime;			// 鼠标时间
	IBKLinePtr							    m_mouseKline;			// 鼠标时间对应的k线

	virtual void							QueryClose() override final;
	virtual void							QueryData(Time_Type timetype)  override final;

	// 计算circle根k线均值 其中newKline是circle中的最后一根
	double									CountAverage(const std::string& codeId, Time_Type timeType, IBKLinePtr newKline, int circle);

	// 将鼠标K线转换为指定的时间类型
	IBKLinePtr								TransMouseKline(Time_Type timeType);



};

