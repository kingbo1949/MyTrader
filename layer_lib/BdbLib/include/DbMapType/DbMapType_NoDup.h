#pragma once
#include "DbMapType.h"

namespace Bdb
{
	class CDbMapType_NoDup;
	typedef IceUtil::Handle<CDbMapType_NoDup> DbMapType_NoDupPtr; 

	class CDbMapType_NoDup : public CDbMapType
	{
	public:
		static DbMapType_NoDupPtr					Instance();
		virtual ~CDbMapType_NoDup(void);

		virtual u_int32_t			MakeFlagForAddOneJob(u_int32_t flag);
		virtual u_int32_t			MakeFlagForGetOneJob();
		virtual u_int32_t			MakeFlagForRemoveOneJob();
		virtual u_int32_t			MakeFlagForDBSetFlag();
		virtual bool				IsDup() {return false;};


	protected:
		CDbMapType_NoDup(void);
		static DbMapType_NoDupPtr					m_pMapTypeNoDup;
	};


}
