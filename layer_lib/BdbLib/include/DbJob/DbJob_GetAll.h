#pragma once
#include "DbJob.h"

namespace Bdb
{
	template<typename KEY, typename VALUE>
	class CDbJob_GetAll : public CDbJob<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_GetAll<KEY, VALUE> >DbJob_GetAllPtr;
		typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;
		typedef typename CDb<KEY, VALUE>::FieldPair FieldPair;
		typedef typename CDb<KEY, VALUE>::FieldPairs FieldPairs;
		typedef typename CDb<KEY, VALUE>::FieldPairRange FieldPairRange;
		typedef CDbJob<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;
	public:

		CDbJob_GetAll(DbPtr pDb)
			:BaseClass(pDb)
		{
		}

		virtual ~CDbJob_GetAll(void)
		{
		}

		FieldPairs			GetResult()
		{
			return m_result;
		}

	protected:
		FieldPairs					m_result;

		virtual int		Setup_Cursor()
		{
			int err = m_pDB->GetDb()->cursor(NULL, &m_cursor, 0);
			return err;
		}

		virtual int		RealExecute()
		{
			Dbt keyDbt, valueDbt;
			int err = 0;
			int loop = 0;
			while ((err = m_cursor->get(&keyDbt, &valueDbt, DB_NEXT)) == 0 && ++loop < CURSOR_MAX_LOOP)
			{
				KEY key;
				VALUE value;
				UnMarshalObject<KEY>(keyDbt, key);
				UnMarshalObject<VALUE>(valueDbt, value);
				m_result.push_back(make_pair(key, value));

			}

			return HandleErrNo(err);


		}


	};

}
