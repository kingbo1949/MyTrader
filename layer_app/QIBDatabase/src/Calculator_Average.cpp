#include "Calculator_Average.h"

void CCalculator_Average::UpdateValuesToDb(const std::string& codeId, ITimeType timeType, const IAvgValues& values)
{
	for (const auto& value : values)
	{
		UpdateToDb(codeId, timeType, value);
	}

}
