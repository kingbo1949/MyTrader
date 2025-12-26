#pragma once




#include <db_cxx.h>
#include <QStruc.h>
using namespace Ice;
using namespace IBTrader;
#include "GlobalDefine.h"

namespace Bdb
{
	class CCompare
	{
	public:
		CCompare(void);
		virtual ~CCompare(void);

		static int Relative(const Long& value1, const Long& value2);
		static int LongA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp);

		static int Relative(const std::string& value1, const std::string& value2);
		static int CodeIdA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp);

		static int Relative(const ITick& value1, const ITick& value2);
		static int TickA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp);

		static int Relative(const IQKey& value1, const IQKey& value2);
		static int QKeyA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp);

		static int Relative(const IKLine& value1, const IKLine& value2);
		static int KLineA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp);

		static int Relative(const IAvgValue& value1, const IAvgValue& value2);
		static int AvgValueA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp);

		static int Relative(const IMacdValue& value1, const IMacdValue& value2);
		static int MacdA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp);

		static int Relative(const IDivTypeValue& value1, const IDivTypeValue& value2);
		static int DivTypeA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp);


	};


}

