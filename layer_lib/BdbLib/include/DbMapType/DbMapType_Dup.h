#pragma once
#include "DbMapType.h"

namespace Bdb
{
	class CDbMapType_Dup;
	typedef IceUtil::Handle<CDbMapType_Dup> DbMapType_DupPtr; 

	class CDbMapType_Dup : public CDbMapType
	{
	public:
		static DbMapType_DupPtr					Instance();
		virtual ~CDbMapType_Dup(void);

		virtual u_int32_t			MakeFlagForAddOneJob(u_int32_t flag);
		virtual u_int32_t			MakeFlagForGetOneJob();
		virtual u_int32_t			MakeFlagForRemoveOneJob();
		virtual u_int32_t			MakeFlagForDBSetFlag();
		virtual bool				IsDup() {return true;};


	protected:
		CDbMapType_Dup(void);
		static DbMapType_DupPtr					m_pMapTypeDup;

	};


}
