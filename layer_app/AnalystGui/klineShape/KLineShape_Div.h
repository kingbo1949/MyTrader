#pragma once
#include "KLineShape.h"

// 判断背离形态，背离的定义：
// 1. 从输入的endtime开始，向前查找指定的N个K线，N由周期和品种决定
// 2. 如果查看顶部背离，则底部不能出现任何信号；如果查看底部背离，则顶部不能出现任何信号。这两种叫做发现了不兼容的信号。
// 3. 一直往前回溯，直到找到一个指定的背离信号为止
// 4. 如果找到，继续往前回溯，碰到不兼容的信号则停止，返回初等背离信号的位置。如果没有碰到不兼容的信号，就找到了第二个指定信号，则
//    判断两个信号之间的距离，如果小于等于5则返回双重背离状态，如果足够远，则是简单背离状态。

enum class MeetStatus
{
	NoMeet,						// 尚未遇见任何信号
	MeetOne,					// 已遇见一个信号
	MeetTwo						// 已遇见两个信号
};
struct ScanState
{
	MeetStatus		meetStatus = MeetStatus::NoMeet;
	int				fstPos = 0;		// 第一个信号位置
	int				scdPos = 0;		// 第二个信号位置
};

class CKLineShape_Div : public CKLineShape
{
public:
	CKLineShape_Div(const KShapeInput& kShapeInput);
	virtual ~CKLineShape_Div() { ; };

	virtual KShapeOutputs		Execute() override final;

protected:
	// 检查单个品种的背离形态
	KShape						checkOneCodeId(IbContractPtr contract, int& pos);

	// 查询指定时间之前的N个数据
	IBDivTypePtrs				QueryDivTypeValues(IbContractPtr contract, Time_Type timeType, Tick_T endTime);

	// 根据最终状态返回背离形态
	KShape						MakeKShapeFromState(const ScanState& finalScanState, int& pos);

};

