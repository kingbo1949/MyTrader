#include "KLineShape_Div.h"
#include <Factory_QDatabase.h>
CKLineShape_Div::CKLineShape_Div(const KShapeInput& kShapeInput)
	:CKLineShape(kShapeInput)
{
}

KShapeOutputs CKLineShape_Div::Execute()
{
	KShapeOutputs ret;
	for (auto contract : m_kShapeInput.contracts)
	{
		int pos = 0;

		KShape kShape = checkOneCodeId(contract, pos);
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

KShape CKLineShape_Div::checkOneCodeId(IbContractPtr contract, int& pos)
{

	ScanState scanState;
	KShape ret = KShape::Normal;

	IBDivTypePtrs divTypeValues = QueryDivTypeValues(contract, m_kShapeInput.timeType, m_kShapeInput.endTime);
	for (int i = int(divTypeValues.size()) - 1; i >= 0; --i)
	{
		pos = int(divTypeValues.size() - 1 - i);

		if (!IsCompatible(m_kShapeInput.topOrBottom, divTypeValues[i]->divType)) 
		{
			// 遇见不兼容的信号，停止
			break;
		}

		if (IsMeetSignal(m_kShapeInput.topOrBottom, divTypeValues[i]))
		{
			if (scanState.meetStatus == MeetStatus::NoMeet)
			{
				// 第一个信号
				scanState.meetStatus = MeetStatus::MeetOne;
				scanState.fstPos = pos;
			}
			else if (scanState.meetStatus == MeetStatus::MeetOne)
			{
				// 第二个信号
				scanState.meetStatus = MeetStatus::MeetTwo;
				scanState.scdPos = pos;
				// 找到两个信号后停止
				break;
			}
		}


	}
	// 根据最终状态返回背离形态
	ret = MakeKShapeFromState(scanState, pos);

	return ret;
}

IBDivTypePtrs CKLineShape_Div::QueryDivTypeValues(IbContractPtr contract, Time_Type timeType, Tick_T endTime)
{
	// 查询K线需要回溯多少数据
	int backCount = CheckBackKLineCount(timeType, contract->securityType);


	QQuery query;
	query.query_type = QQueryType::BEFORE_TIME;
	query.time_ms = endTime + 1;
	query.query_number = backCount;
	IBDivTypePtrs divTypeValues = MakeAndGet_QDatabase()->GetDivTypes(contract->codeId, timeType, query);
	return divTypeValues;
}

KShape CKLineShape_Div::MakeKShapeFromState(const ScanState& finalScanState, int& pos)
{
	KShape ret = KShape::Normal;
	if (finalScanState.meetStatus == MeetStatus::NoMeet)  return ret;

	if (finalScanState.meetStatus == MeetStatus::MeetOne)
	{
		// 简单背离
		ret = KShape::Div;
		pos = finalScanState.fstPos ;
	}
	if (finalScanState.meetStatus == MeetStatus::MeetTwo)
	{
		// 双重背离
		int distance = finalScanState.scdPos - finalScanState.fstPos;
		if (distance <= 5)
		{
			ret = KShape::DblDivPlus;
		}
		else
		{
			ret = KShape::DblDiv;
		}
		pos = finalScanState.fstPos;
	}
	return ret;
}
