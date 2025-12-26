#include <DbMapType/DbMapType_NoDup.h>


DbMapType_NoDupPtr CDbMapType_NoDup::m_pMapTypeNoDup = NULL;
DbMapType_NoDupPtr CDbMapType_NoDup::Instance()
{
	if(!m_pMapTypeNoDup)
	{
		m_pMapTypeNoDup = new CDbMapType_NoDup();
	}
	return m_pMapTypeNoDup;
}

CDbMapType_NoDup::CDbMapType_NoDup(void)
{
}

CDbMapType_NoDup::~CDbMapType_NoDup(void)
{
}
u_int32_t CDbMapType_NoDup::MakeFlagForAddOneJob(u_int32_t flag)
{
	return flag;
}
u_int32_t CDbMapType_NoDup::MakeFlagForGetOneJob()
{
	printf("return set \n");
	return DB_SET;
}
u_int32_t CDbMapType_NoDup::MakeFlagForRemoveOneJob()
{
	return DB_SET;
}
u_int32_t CDbMapType_NoDup::MakeFlagForDBSetFlag()
{
	return 0;
}