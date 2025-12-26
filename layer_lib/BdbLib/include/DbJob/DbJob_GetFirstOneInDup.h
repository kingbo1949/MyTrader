#pragma once
#include "DbJob.h"

namespace Bdb
{
	template<typename KEY, typename VALUE>
	class CDbJob_GetFirstOneInDup : public CDbJob<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_GetFirstOneInDup<KEY, VALUE> >DbJob_GetFirstOneInDupPtr;
		typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;
		typedef typename CDb<KEY, VALUE>::FieldPair FieldPair;
		typedef CDbJob<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;
	public:

		CDbJob_GetFirstOneInDup(DbPtr pDb, const KEY& keyIn)
			:BaseClass(pDb), m_keyIn(keyIn), m_validResult(false)
		{
		}

		virtual ~CDbJob_GetFirstOneInDup(void)
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
			Ice::ByteSeq keybuff;
			Dbt keyDbt;
			MarshalObject(keybuff, m_keyIn, keyDbt);

			Dbt	valueDbt;
			// 查找位置
			int err = m_cursor->get(&keyDbt, &valueDbt, DB_SET);
			if (!err)
			{
				m_validResult = true;
				UnMarshalObject<VALUE>(valueDbt, m_result);


			}
			return HandleErrNo(err);

		}


	};

}

