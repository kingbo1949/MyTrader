#pragma once
#include "DbJob.h"

namespace Bdb
{
	class CCompare;
	template<typename KEY, typename VALUE, class CmpClass = CCompare>
	class CDbJob_GetRangeKey : public CDbJob<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_GetRangeKey<KEY, VALUE> >DbJob_GetRangeKeyPtr;
		typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;
		typedef typename CDb<KEY, VALUE>::FieldPair FieldPair;
		typedef typename CDb<KEY, VALUE>::FieldKeyRange FieldKeyRange;
		typedef CDbJob<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;
	public:

		CDbJob_GetRangeKey(DbPtr pDb, const FieldKeyRange& keyRange)
			:BaseClass(pDb), m_keyRange(keyRange)
		{
		}

		virtual ~CDbJob_GetRangeKey(void)
		{
		}

		std::vector<KEY>			GetResult()
		{
			return m_result;
		}

	protected:
		FieldKeyRange				m_keyRange;
		std::vector<KEY>					m_result;


		virtual int		Setup_Cursor()
		{
			int err = m_pDB->GetDb()->cursor(NULL, &m_cursor, 0);
			return err;
		}

		virtual int		RealExecute()
		{
			Ice::ByteSeq keybuff;
			Dbt keyDbt;
			MarshalObject(keybuff, m_keyRange.beginKey, keyDbt);

			Dbt	valueDbt;

			// 查找第一个位置
			int err = m_cursor->get(&keyDbt, &valueDbt, DB_SET_RANGE);
			if (err) return HandleErrNo(err);

			if (!CurrentCheckInRangeAndNeedContinue(keyDbt)) return 0;

			int loop = 0;
			while ((err = m_cursor->get(&keyDbt, &valueDbt, GetNextFlag())) == 0 && ++loop < CURSOR_MAX_LOOP)
			{
				if (!CurrentCheckInRangeAndNeedContinue(keyDbt)) return 0;
			}
			return HandleErrNo(err);
		}

		bool CurrentCheckInRangeAndNeedContinue(const Dbt& keyDbt)
		{
			KEY key;
			UnMarshalObject<KEY>(keyDbt, key);

			if (InRange_Key(key))
			{
				m_result.push_back(key);
				return true;
			}
			else
			{
				return false;
			}
		}

		bool InRange_Key(const KEY& currentKey)
		{
			if (CmpClass::Relative(currentKey, m_keyRange.beginKey) == -1) return false;
			if (CmpClass::Relative(currentKey, m_keyRange.endKey) == 0) return false;
			if (CmpClass::Relative(currentKey, m_keyRange.endKey) == 1) return false;

			return true;


		}

		u_int32_t GetNextFlag()
		{
			if (m_pDB->IsDup())
			{
				return DB_NEXT_NODUP;
			}
			else
			{
				return DB_NEXT;
			}

		}
	};
}
