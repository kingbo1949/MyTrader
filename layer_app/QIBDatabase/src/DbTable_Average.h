#pragma once
#include "GlobalDefine.h"

namespace Bdb
{
	class CDbTable_Average : public CDbTable
	{
	public:
		CDbTable_Average(DbEnv* env, const std::string& path, const std::string& dbName);
		virtual ~CDbTable_Average(void) { ; };

		virtual	int				Flush()
		{
			return m_pDb->Flush();
		}
		virtual	bool	SelfCheck()
		{
			return m_pDb->SelfCheck(m_dbName);
		}

		void		AddOne(const std::string& codeId, ITimeType timeType, const IAvgValue& value);
		void		AddSome(const std::string& codeId, ITimeType timeType, const IAvgValues& values);
		bool		GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IAvgValue& value);

		void		RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos);
		void		RemoveKey(const std::string& codeId, ITimeType timeType);
		void		RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime);
		void		RemoveAll();

		void		GetValues(const std::string& codeId, ITimeType timeType, const IQuery& query, IAvgValues& values);





	protected:
		CDb<IQKey, IAvgValue>::DbPtr	m_pDb;

		// 取值范围[beginDayTime,endDayTime)
		void		GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IAvgValues& values);
		void		GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IAvgValues& values);
		void		GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IAvgValues& values);
	};
	typedef IceUtil::Handle<CDbTable_Average> DbTable_AveragePtr;

}






