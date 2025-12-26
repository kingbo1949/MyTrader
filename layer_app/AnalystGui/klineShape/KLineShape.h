#pragma once
#include <memory>
#include <base_struc.h>
#include "KShapeDefine.h"

struct KShapeInput
{
	IbContractPtrs		contracts;
	Time_Type			timeType;
	TopOrBottom			topOrBottom;
	Tick_T				endTime;
	std::set<KShape>	allowedList = { KShape::DblDiv, KShape::DblDivPlus, KShape::Div, KShape::TrainDiv };   // 允许的背离形态列表

};


class CKLineShape
{
public:
	CKLineShape(const KShapeInput& kShapeInput) ;
	virtual ~CKLineShape() { ; };

	virtual KShapeOutputs		Execute() = 0;

protected:
	KShapeInput					m_kShapeInput;

};
typedef std::shared_ptr<CKLineShape> KLineShapePtr;
typedef std::vector<KLineShapePtr> KLineShapePtrs;


