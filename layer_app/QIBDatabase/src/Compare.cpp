#include "Compare.h"
#include <Field.h>
#include <Global.h>
Bdb::CCompare::CCompare(void)
{

}

Bdb::CCompare::~CCompare(void)
{

}

int Bdb::CCompare::Relative(const std::string& value1, const std::string& value2)
{
	return value1.compare(value2);
}



int Bdb::CCompare::CodeIdA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp)
{
	locp = NULL;
	std::string value1, value2;
	UnMarshalObject<std::string>(*dbt1, value1);
	UnMarshalObject<std::string>(*dbt2, value2);
	return Relative(value1, value2);

}

int Bdb::CCompare::Relative(const Long& value1, const Long& value2)
{
	if (value1 > value2) return 1;
	if (value1 < value2) return -1;

	return 0;

}
int Bdb::CCompare::LongA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp)
{
	locp = NULL;

	Long value1, value2;
	UnMarshalObject<Long>(*dbt1, value1);
	UnMarshalObject<Long>(*dbt2, value2);
	return Relative(value1, value2);

}


int Bdb::CCompare::Relative(const ITick& value1, const ITick& value2)
{
	if (value1.time > value2.time) return 1;
	if (value1.time < value2.time) return -1;

	return 0;

}
int Bdb::CCompare::TickA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp)
{
	locp = NULL;
	ITick value1, value2;
	UnMarshalObject<ITick>(*dbt1, value1);
	UnMarshalObject<ITick>(*dbt2, value2);

	return Relative(value1, value2);

}

int Bdb::CCompare::Relative(const IQKey& value1, const IQKey& value2)
{
	if (value1.codeId != value2.codeId)
	{
		if (value1.codeId > value2.codeId) return 1;
		if (value1.codeId < value2.codeId) return -1;
	}

	if (value1.timeType > value2.timeType) return 1;
	if (value1.timeType < value2.timeType) return -1;

	return 0;


}
int Bdb::CCompare::QKeyA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp)
{
	locp = NULL;
	IQKey value1, value2;
	UnMarshalObject<IQKey>(*dbt1, value1);
	UnMarshalObject<IQKey>(*dbt2, value2);
	return Relative(value1, value2);

}
int Bdb::CCompare::Relative(const IKLine& value1, const IKLine& value2)
{
	if (value1.time > value2.time) return 1;
	if (value1.time < value2.time) return -1;

	return 0;

}

int Bdb::CCompare::KLineA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp)
{
	locp = NULL;
	IKLine value1, value2;
	UnMarshalObject<IKLine>(*dbt1, value1);
	UnMarshalObject<IKLine>(*dbt2, value2);

	return Relative(value1, value2);

}


int Bdb::CCompare::Relative(const IAvgValue& value1, const IAvgValue& value2)
{
	if (value1.time > value2.time) return 1;
	if (value1.time < value2.time) return -1;
	return 0;


}
int Bdb::CCompare::AvgValueA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp)
{
	locp = NULL;
	IAvgValue value1, value2;
	UnMarshalObject<IMaVaue>(*dbt1, value1);
	UnMarshalObject<IMaVaue>(*dbt2, value2);

	return Relative(value1, value2);

}

int Bdb::CCompare::Relative(const IMacdValue& value1, const IMacdValue& value2)
{
	if (value1.time > value2.time) return 1;
	if (value1.time < value2.time) return -1;
	return 0;
}

int Bdb::CCompare::MacdA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp)
{
	locp = NULL;
	IMacdValue value1, value2;
	UnMarshalObject<IMacdValue>(*dbt1, value1);
	UnMarshalObject<IMacdValue>(*dbt2, value2);

	return Relative(value1, value2);
}

int Bdb::CCompare::Relative(const IDivTypeValue& value1, const IDivTypeValue& value2)
{
	if (value1.time > value2.time) return 1;
	if (value1.time < value2.time) return -1;
	return 0;
}

int Bdb::CCompare::DivTypeA(Db* dbp, const Dbt* dbt1, const Dbt* dbt2, size_t* locp)
{
	locp = NULL;
	IDivTypeValue value1, value2;
	UnMarshalObject<IDivTypeValue>(*dbt1, value1);
	UnMarshalObject<IDivTypeValue>(*dbt2, value2);

	return Relative(value1, value2);
}

int CCompare::Relative(const IAtrValue &value1, const IAtrValue &value2)
{
	if (value1.time > value2.time) return 1;
	if (value1.time < value2.time) return -1;
	return 0;
}

int CCompare::AtrValueA(Db *dbp, const Dbt *dbt1, const Dbt *dbt2, size_t *locp)
{
	locp = NULL;
	IAtrValue value1, value2;
	UnMarshalObject<IAtrValue>(*dbt1, value1);
	UnMarshalObject<IAtrValue>(*dbt2, value2);

	return Relative(value1, value2);
}


