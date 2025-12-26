#pragma once
#include "Simulator.h"
#include "SimulatorDefine.h"
class CSimulator_Real : public CSimulator
{
public:
	CSimulator_Real() ;
	virtual ~CSimulator_Real() { ; };

	// 撮合
	virtual void			MeetDeal() override final;

	// 收到委托
	virtual void			RecOneOrder(OneOrderOfSimulatorPtr pOrder) override final;

	// 收到撤单
	virtual void			RecOneCancel(int localOrderNo) override final;

protected:
	std::mutex									m_mutex;
	std::map<LocalOrderNo, OneOrderOfSimulatorPtr>	m_orders;

	LineType									m_lineType;


	// 撮合
	bool										MeetDeal(OneOrderOfSimulatorPtr porder, double& dealPrice);







};

