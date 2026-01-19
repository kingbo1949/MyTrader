#include "KLineShape.h"

CKLineShape::CKLineShape(const KShapeInput& kShapeInput)
	:m_kShapeInput(kShapeInput)
{
}

KShapeOutputs CKLineShape::Execute()
{
	KShapeOutputs ret;
	for (auto contract : m_kShapeInput.contracts)
	{
		int pos = 0;

		KShape kShape = CheckOneCodeId(contract, pos);
		// 不是需要的形态则跳过
		if (m_kShapeInput.allowedList.find(kShape) == m_kShapeInput.allowedList.end()) continue;

		KShapeOutput output;
		output.codeId = contract->codeId;
		output.timeType = m_kShapeInput.timeType;
		output.topOrBottom = m_kShapeInput.topOrBottom;
		output.kShape = kShape;
		output.signalPosition = pos;
		ret.push_back(output);
	}
	return ret;

}
 