#pragma once
#include "TickBoard.h"
class CTickBoard_Real : public CTickBoard
{
public:
	CTickBoard_Real() ;
	virtual ~CTickBoard_Real() { ; };

	virtual TickBoardStatus		GetTickBoardStatus() override final;

	virtual void 				SetTickBoardStatus(TickBoardStatus status)  override final;

	virtual CodeHashId			GetCodeHash() override final;

	virtual void				SetCodeHash(CodeHashId codeHash) override final;

	// 更新
	virtual void				AddOne(IBTickPtr tick)  override final;

	// 收到全部ticks
	virtual IBTickPtrs			ReceiveAll()  override final;

protected:
	TickBoardStatus				m_status;

	CodeHashId					m_codeHash;

	IBTickPtrs					m_ticks;

	// 整理m_ticks的内容，从回调接口仅返回了时间、价格和数量，需要把其他字段都填上
	void						RefillAllTicks();
};

