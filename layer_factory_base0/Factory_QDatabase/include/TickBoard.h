#pragma once

#include <base_struc.h>

// 查询历史数据时，用来接收回调返回的tick,，串行执行，
enum class TickBoardStatus
{
	Ready,				// 就绪初始态
	Busy,				// 已经发送了查询请求，处于接收查询结果的状态
	Done				// 查询结果已经全部返回完毕，但是还未被取走
};

class CTickBoard
{
public:
	CTickBoard() { ; };
	virtual ~CTickBoard() { ; };

	virtual TickBoardStatus		GetTickBoardStatus() = 0;

	virtual void 				SetTickBoardStatus(TickBoardStatus status) = 0;

	virtual CodeHashId			GetCodeHash() = 0;

	virtual void				SetCodeHash(CodeHashId codeHash) = 0;

	// 更新
	virtual void				AddOne(IBTickPtr tick) = 0;

	// 收到全部ticks
	virtual IBTickPtrs			ReceiveAll() = 0;

};
typedef std::shared_ptr<CTickBoard> TickBoardPtr;


