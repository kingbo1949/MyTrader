#pragma once



#include <db_cxx.h>
#include <IceUtil/IceUtil.h>
namespace Bdb
{
	class CDbMapType : public IceUtil::Shared
	{
	public:
		CDbMapType(void);
		virtual ~CDbMapType(void);

		virtual u_int32_t			MakeFlagForAddOneJob(u_int32_t flag) = 0;
		virtual u_int32_t			MakeFlagForGetOneJob() = 0;
		virtual u_int32_t			MakeFlagForRemoveOneJob() = 0;
		virtual u_int32_t			MakeFlagForDBSetFlag() = 0;
		virtual bool				IsDup() = 0;


	};
	typedef IceUtil::Handle<CDbMapType> DbMapTypePtr;


}
using namespace Bdb;

