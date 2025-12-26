#pragma once
#include "KLineBoard.h"
class CKLineBoard_Real : public CKLineBoard
{
public:
	CKLineBoard_Real() ;
	virtual ~CKLineBoard_Real() { ; };

	virtual KLineBoardStatus	GetKLineBoardStatus() override final;

	virtual void 				SetKLineBoardStatus(KLineBoardStatus status) override final;

	// 更新
	virtual void				AddOne(IBKLinePtr kline) override final;

	// 收到全部k线
	virtual IBKLinePtrs			ReceiveAll() override final;

protected:
	KLineBoardStatus			m_status;

	IBKLinePtrs					m_klines;
};

