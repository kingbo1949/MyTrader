#include <DbMapType/DbMapType_Dup.h>



DbMapType_DupPtr CDbMapType_Dup::m_pMapTypeDup = NULL;
DbMapType_DupPtr CDbMapType_Dup::Instance()
{
	if(!m_pMapTypeDup)
	{
		m_pMapTypeDup = new CDbMapType_Dup();
	}
	return m_pMapTypeDup;
}


CDbMapType_Dup::CDbMapType_Dup(void)
{
}

CDbMapType_Dup::~CDbMapType_Dup(void)
{
}
u_int32_t CDbMapType_Dup::MakeFlagForAddOneJob(u_int32_t flag)
{
	return flag|DB_OVERWRITE_DUP;
}
u_int32_t CDbMapType_Dup::MakeFlagForGetOneJob()
{
	return DB_GET_BOTH;
}
u_int32_t CDbMapType_Dup::MakeFlagForRemoveOneJob()
{
	return DB_GET_BOTH;
}
u_int32_t CDbMapType_Dup::MakeFlagForDBSetFlag()
{
	return DB_DUPSORT;
}