#pragma once

#include <DbTable.h>
#include <QStruc.h>
using namespace IBTrader;

namespace Bdb
{
	class CDbTable_TickHis : public CDbTable
	{
	public:
		CDbTable_TickHis(DbEnv* env, const std::string& path, const std::string& dbName);
		virtual ~CDbTable_TickHis(void);

		virtual	int				Flush()
		{
			return m_pDb->Flush();
		}
		virtual	bool	SelfCheck()
		{
			return m_pDb->SelfCheck(m_dbName);
		}

		void		AddOne(const ITick& value);
		void		AddSome(const ITicks& values);
		bool		GetOne(const std::string& codeId, Long timePos, ITick& value);
		void		RemoveOne(const std::string& codeId, Long timePos);
		void		RemoveKey(const std::string& codeId);

		void		RemoveByRange(const std::string& codeId, Long beginTime, Long endTime);
		void		GetTicks(const std::string& codeId, const IQuery& query, ITicks& values);
		void		RemoveAll();
		bool		GetLastTick(const std::string& codeId, ITick& value);



	protected:
		CDb<std::string, ITick>::DbPtr	m_pDb;

		// 取值范围[beginDayTime,endDayTime)
		void		GetRange(const std::string& codeId, Long beginTime, Long endTime, ITicks& values);
		void		GetForWard(const std::string& codeId, Long beginTime, Long count, ITicks& values);
		void		GetBackWard(const std::string& codeId, Long endTime, Long count, ITicks& values);

	};
	typedef IceUtil::Handle<CDbTable_TickHis> DbTable_TickHisPtr;

}

