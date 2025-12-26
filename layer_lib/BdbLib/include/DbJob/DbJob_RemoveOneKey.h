#pragma once
#include "DbJob.h"

namespace Bdb
{
	template<typename KEY, typename VALUE>
	class CDbJob_RemoveOneKey : public CDbJob<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_RemoveOneKey<KEY, VALUE> >DbJob_RemoveOneKeyPtr;
		typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;
		typedef typename CDb<KEY, VALUE>::FieldPair FieldPair;
		typedef CDbJob<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;
	public:

		CDbJob_RemoveOneKey(DbPtr pDb, const KEY& keyIn)
			:BaseClass(pDb), m_keyIn(keyIn)
		{
		}

		virtual ~CDbJob_RemoveOneKey(void)
		{
		}

	protected:
		KEY					m_keyIn;

		virtual int		Setup_Cursor()
		{
			return 0;
		}

		virtual int		RealExecute()
		{
			Ice::ByteSeq keybuff;
			Dbt keyDbt;
			MarshalObject(keybuff, m_keyIn, keyDbt);

			int err = m_pDB->GetDb()->del(NULL, &keyDbt, 0);
			return HandleErrNo(err);
		}

	};

}

