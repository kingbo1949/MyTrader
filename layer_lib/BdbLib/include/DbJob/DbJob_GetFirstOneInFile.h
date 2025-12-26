#pragma once

#include "DbJob.h"

namespace Bdb
{
	template<typename KEY, typename VALUE>
	class CDbJob_GetFirstOneInFile : public CDbJob<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_GetFirstOneInFile<KEY, VALUE> >DbJob_GetFirstOneInFilePtr;
		typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;
		typedef typename CDb<KEY, VALUE>::FieldPair FieldPair;
		typedef CDbJob<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;
	public:

		CDbJob_GetFirstOneInFile(DbPtr pDb)
			:BaseClass(pDb), m_validResult(false)
		{
		}

		virtual ~CDbJob_GetFirstOneInFile(void)
		{
		}

		bool 				GetResult(KEY& key, VALUE& value)
		{
			key = m_resultKey;
			value = m_resultValue;
			return m_validResult;
		}

	protected:
		bool				m_validResult;
		KEY					m_resultKey;
		VALUE				m_resultValue;

		virtual int					Setup_Cursor()
		{
			int err = m_pDB->GetDb()->cursor(NULL, &m_cursor, 0);
			return err;

		}
		virtual int					RealExecute()
		{

			Dbt keyDbt, valueDbt;
			// 查找位置
			int err = m_cursor->get(&keyDbt, &valueDbt, DB_FIRST);
			if (!err)
			{
				m_validResult = true;

				UnMarshalObject<KEY>(keyDbt, m_resultKey);
				UnMarshalObject<VALUE>(valueDbt, m_resultValue);

			}
			return HandleErrNo(err);

		}




	};

}

