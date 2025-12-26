#pragma once
#include "DbJob.h"
namespace Bdb
{
	template<typename KEY, typename VALUE>
	class CDbJob_GetLastOneInDup : public CDbJob<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_GetLastOneInDup<KEY, VALUE> >DbJob_GetLastOneInDupPtr;
		typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;
		typedef typename CDb<KEY, VALUE>::FieldPair FieldPair;
		typedef CDbJob<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;
	public:

		CDbJob_GetLastOneInDup(DbPtr pDb, const KEY& keyIn)
			:BaseClass(pDb), m_keyIn(keyIn), m_validResult(false)
		{
		}

		virtual ~CDbJob_GetLastOneInDup(void)
		{
		}

		bool 				GetResult(VALUE& value)
		{
			value = m_result;
			return m_validResult;
		}
	protected:
		KEY					m_keyIn;

		bool				m_validResult;
		VALUE				m_result;

		virtual int					Setup_Cursor()
		{
			int err = m_pDB->GetDb()->cursor(NULL, &m_cursor, 0);
			return err;

		}
		virtual int					RealExecute()
		{
			int err = GetFirstPos();
			if (err) return HandleErrNo(err);

			err = GetNextKey();
			if (!err) return GetPrePos();


			if (err && err == DB_NOTFOUND)
			{
				return GetLastPosInDB();
			}

			return HandleErrNo(err);

		}

		int GetFirstPos()
		{
			Ice::ByteSeq keybuff;
			Dbt keyDbt;
			MarshalObject(keybuff, m_keyIn, keyDbt);

			Dbt	valueDbt;
			return m_cursor->get(&keyDbt, &valueDbt, DB_SET);
		}

		int GetNextKey()
		{
			Dbt keyDbt;
			Dbt	valueDbt;

			int ret = m_cursor->get(&keyDbt, &valueDbt, DB_NEXT_NODUP);
			return ret;
		}

		int GetPrePos()
		{
			Dbt keyDbt;
			Dbt	valueDbt;

			int err = m_cursor->get(&keyDbt, &valueDbt, DB_PREV);
			if (!err)
			{
				m_validResult = true;

				UnMarshalObject<VALUE>(valueDbt, m_result);
			}
			return err;
		}

		int	GetLastPosInDB()
		{
			Dbt keyDbt;
			Dbt	valueDbt;

			int err = m_cursor->get(&keyDbt, &valueDbt, DB_LAST);
			if (!err)
			{
				m_validResult = true;
				UnMarshalObject<VALUE>(valueDbt, m_result);

			}
			return err;

		}
	};
}
