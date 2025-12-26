#pragma once

#include <base_struc.h>
// 查询历史数据时，用来接收回调返回的Kline,，串行执行，


enum class KLineBoardStatus
{
	Ready,				// 就绪初始态
	Busy,				// 已经发送了查询请求，处于接收查询结果的状态
	Done				// 查询结果已经全部返回完毕，但是还未被取走
};


class CKLineBoard
{
public:
	CKLineBoard() { ; };
	virtual ~CKLineBoard() { ; };

	virtual KLineBoardStatus	GetKLineBoardStatus() = 0;

	virtual void 				SetKLineBoardStatus(KLineBoardStatus status) = 0;


	// 更新
	virtual void				AddOne(IBKLinePtr kline) = 0;

	// 收到全部k线
	virtual IBKLinePtrs			ReceiveAll() = 0;

};
typedef std::shared_ptr<CKLineBoard> KLineBoardPtr;



