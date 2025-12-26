#include "pch.h"
#include "OnRtnQueue_Real.h"

void COnRtnQueue_Real::AddOne(OnRtnPtr pOne)
{
	m_onRtnQueue.enqueue(pOne);
	return;

}

OnRtnPtr COnRtnQueue_Real::PopOne()
{
	OnRtnPtr back;
	if (!m_onRtnQueue.try_dequeue(back)) return nullptr;
	return back;

}
