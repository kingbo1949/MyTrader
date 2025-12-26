#pragma once
#include "DaemonByTick_Real.h"
#include <base_struc.h>
class CDaemonByTick_QGenerator : public CDaemonByTick_Real
{
public:
	CDaemonByTick_QGenerator();
	virtual ~CDaemonByTick_QGenerator() { ; };

	virtual void		Execute() override final;

	void				UpdateDbSecond();

protected:
	time_t				m_lastUpdateDbSeccond;
};
typedef std::shared_ptr<CDaemonByTick_QGenerator> DaemonByTick_QGeneratorPtr;


DaemonByTick_QGeneratorPtr	MakeAndGet_DaemonByTick_QGenerator();
