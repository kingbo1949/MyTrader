#include "pch.h"
#include "KLineBoard_Real.h"
#include <Factory_Log.h>
CKLineBoard_Real::CKLineBoard_Real()
	:m_status(KLineBoardStatus::Ready)
{

}

KLineBoardStatus CKLineBoard_Real::GetKLineBoardStatus()
{
	return m_status;
}

void CKLineBoard_Real::SetKLineBoardStatus(KLineBoardStatus status)
{
	m_status = status;
}

void CKLineBoard_Real::AddOne(IBKLinePtr kline)
{
	if (m_status != KLineBoardStatus::Busy)
	{
		Log_Print(LogLevel::Err, "CKLineBoard_Real::AddOne, m_status != KLineBoardStatus::Busy!!");
		exit(-1);
	}

	m_klines.push_back(kline);
	return;
}


IBKLinePtrs CKLineBoard_Real::ReceiveAll()
{
	if (m_status != KLineBoardStatus::Done)
	{
		Log_Print(LogLevel::Err, "CKLineBoard_Real::ReceiveAll, m_status != KLineBoardStatus::Done!!");
		exit(-1);
	}
	IBKLinePtrs back;
	m_klines.swap(back);

	SetKLineBoardStatus(KLineBoardStatus::Ready);

	return back;

}
