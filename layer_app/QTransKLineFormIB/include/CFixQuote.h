#pragma once

#include <base_struc.h>
class CFixQuote
{
public:
	CFixQuote(IbContractPtr contract, const TimePair& timePair);
	virtual ~CFixQuote() { ; };

	void		Go();

protected:
	IbContractPtr	m_contract;
	TimePair		m_timePair;

	void			FixOneType(Time_Type timeType);


};

