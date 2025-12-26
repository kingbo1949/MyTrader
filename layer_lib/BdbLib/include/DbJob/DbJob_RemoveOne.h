#pragma once
#include "DbJob.h"

namespace Bdb
{
	template<typename KEY, typename VALUE>
	class CDbJob_RemoveOne : public CDbJob<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_RemoveOne<KEY, VALUE> >DbJob_RemoveOnePtr;
		typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;
		typedef typename CDb<KEY, VALUE>::FieldPair FieldPair;
		typedef CDbJob<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;
	public:

		CDbJob_RemoveOne(DbPtr pDb, const FieldPair& fieldPair)
			:BaseClass(pDb), m_fieldPair(fieldPair)
		{
		}

		virtual ~CDbJob_RemoveOne(void)
		{
		}

	protected:
		FieldPair					m_fieldPair;

		virtual int		Setup_Cursor()
		{
			int err = m_pDB->GetDb()->cursor(NULL, &m_cursor, DB_WRITECURSOR);
			return err;
		}

		virtual int		RealExecute()
		{
			Ice::ByteSeq keybuff;
			Dbt keyDbt;
			MarshalObject(keybuff, m_fieldPair.first, keyDbt);

			Ice::ByteSeq valuebuff;
			Dbt	valueDbt;
			MarshalObject(valuebuff, m_fieldPair.second, valueDbt);

			// 查找位置
			int err = m_cursor->get(&keyDbt, &valueDbt, m_pDB->MakeFlagForRemoveOneJob());
			if (!err)
			{
				return m_cursor->del(0);
			}
			return HandleErrNo(err);


		}

	};

}

