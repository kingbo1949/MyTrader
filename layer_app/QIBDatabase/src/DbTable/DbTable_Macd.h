#pragma once
#include "../GlobalDefine.h"
namespace Bdb
{
	class CDbTable_Macd : public CDbTable
	{
	public:
		CDbTable_Macd(DbEnv* env, const std::string& path, const std::string& dbName);
		virtual ~CDbTable_Macd(void) { ; };

		virtual	int				Flush()
		{
			return m_pDb->Flush();
		}
		virtual	bool	SelfCheck()
		{
			return m_pDb->SelfCheck(m_dbName);
		}

		void		AddOne(const std::string& codeId, ITimeType timeType, const IMacdValue& value);
		void		AddSome(const std::string& codeId, ITimeType timeType, const IMacdValues& values);
		bool		GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IMacdValue& value);

		void		RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos);
		void		RemoveKey(const std::string& codeId, ITimeType timeType);
		void		RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime);
		void		RemoveAll();

		void		GetValues(const std::string& codeId, ITimeType timeType, const IQuery& query, IMacdValues& values);

	protected:
		CDb<IQKey, IMacdValue>::DbPtr	m_pDb;

		// 取值范围[beginDayTime,endDayTime)
		void		GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IMacdValues& values);
		void		GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IMacdValues& values);
		void		GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IMacdValues& values);


	};
	typedef IceUtil::Handle<CDbTable_Macd> DbTable_MacdPtr;

}

