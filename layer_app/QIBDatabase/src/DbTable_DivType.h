#pragma once
#include "GlobalDefine.h"

namespace Bdb
{
	class CDbTable_DivType : public CDbTable
	{
	public:
		CDbTable_DivType(DbEnv* env, const std::string& path, const std::string& dbName);
		virtual ~CDbTable_DivType(void) { ; };

		virtual	int				Flush()
		{
			return m_pDb->Flush();
		}
		virtual	bool	SelfCheck()
		{
			return m_pDb->SelfCheck(m_dbName);
		}

		void		AddOne(const std::string& codeId, ITimeType timeType, const IDivTypeValue& value);
		void		AddSome(const std::string& codeId, ITimeType timeType, const IDivTypeValues& values);
		bool		GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IDivTypeValue& value);

		void		RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos);
		void		RemoveKey(const std::string& codeId, ITimeType timeType);
		void		RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime);
		void		RemoveAll();

		void		GetValues(const std::string& codeId, ITimeType timeType, const IQuery& query, IDivTypeValues& values);

	protected:
		CDb<IQKey, IDivTypeValue>::DbPtr	m_pDb;

		// 取值范围[beginDayTime,endDayTime)
		void		GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IDivTypeValues& values);
		void		GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IDivTypeValues& values);
		void		GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IDivTypeValues& values);


	};
	typedef IceUtil::Handle<CDbTable_DivType> DbTable_DivTypePtr;

}

