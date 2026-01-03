#pragma once
#include <DbTable.h>
#include <QStruc.h>
using namespace IBTrader;

namespace Bdb
{
	class CDbTable_KLine : public CDbTable
	{
	public:
		CDbTable_KLine(DbEnv* env, const std::string& path, const std::string& dbName);
		virtual ~CDbTable_KLine(void) { ; };

		virtual	int				Flush()
		{
			return m_pDb->Flush();
		}
		virtual	bool	SelfCheck()
		{
			return m_pDb->SelfCheck(m_dbName);
		}

		void		AddOne(const std::string& codeId, ITimeType timeType, const IKLine& value);
		void		AddSome(const std::string& codeId, ITimeType timeType, const IKLines& values);
		bool		GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IKLine& value);

		void		RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos);
		void		RemoveKey(const std::string& codeId, ITimeType timeType);
		void		RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime);
		void		RemoveAll();

		void		GetKLines(const std::string& codeId, ITimeType timeType, const IQuery& query, IKLines& values);





	protected:
		CDb<IQKey, IKLine>::DbPtr	m_pDb;

		// 取值范围[beginDayTime,endDayTime)
		void		GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IKLines& values);
		void		GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IKLines& values);
		void		GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IKLines& values);



	};
	typedef IceUtil::Handle<CDbTable_KLine> DbTable_KLinePtr;

}


