#pragma once
#include <QStruc.h>
using namespace IBTrader;
class CMakeKlinePairs
{
public:
	CMakeKlinePairs(const std::string& first, const std::string& second, ITimeType timetype, const IQuery& query);
	virtual ~CMakeKlinePairs(void) { ; };

	IKLinePairs		DoMake();

protected:
	std::string			m_firstCodeId;
	std::string			m_secondCodeId;
	ITimeType		m_timeType;
	IQuery			m_query;

	bool 			DoOnePair(const IKLine& firstKline, IKLinePair& klinepair);



};

