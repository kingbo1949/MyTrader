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

	KShapeOutputs				Execute();

protected:
	KShapeInput					m_kShapeInput;

	// 检查单个品种的背离形态 pos是该形态的位置
	virtual KShape				CheckOneCodeId(IbContractPtr contract, int& pos) = 0;


};
typedef std::shared_ptr<CKLineShape> KLineShapePtr;
typedef std::vector<KLineShapePtr> KLineShapePtrs;


