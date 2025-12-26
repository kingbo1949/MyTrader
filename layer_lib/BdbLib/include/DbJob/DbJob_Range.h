#pragma once

#include "DbJob.h"

namespace Bdb
{
	class CCompare;
	template<typename KEY, typename VALUE, class CmpClass = CCompare>
	class CDbJob_Range : public CDbJob<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_Range<KEY, VALUE> >DbJob_RangePtr;
		typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;
		typedef typename CDb<KEY, VALUE>::FieldPair FieldPair;
		typedef typename CDb<KEY, VALUE>::FieldPairs FieldPairs;
		typedef typename CDb<KEY, VALUE>::FieldPairRange FieldPairRange;
		typedef CDbJob<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;
	public:
		CDbJob_Range(DbPtr pDb, const FieldPairRange& range)
			:BaseClass(pDb), m_range(range)
		{
		}

		virtual ~CDbJob_Range(void)
		{
		}

	protected:
		// 迭代记录的时候判断是否需要继续迭代，为true则需要继续迭代
		// 如果返回false，还需要判断err值，看是否是因为数据库错误导致的停止迭代
		virtual bool CurrentPairCheckInRangeAndNeedContinue(const Dbt& key, const Dbt& value, int& err) = 0;
		virtual int	 Setup_Cursor() = 0;				// 建立光标


		virtual int		RealExecute()
		{
			Ice::ByteSeq keybuff;
			Dbt keyDbt;
			MarshalObject(keybuff, m_range.beginPair.first, keyDbt);

			Ice::ByteSeq valuebuff;
			Dbt	valueDbt;
			MarshalObject(valuebuff, m_range.beginPair.second, valueDbt);

			// 查找第一个位置
			int err = m_cursor->get(&keyDbt, &valueDbt, FindFirstOneFlag());
			if (err) return HandleErrNo(err);

			if (!CurrentPairCheckInRangeAndNeedContinue(keyDbt, valueDbt, err))
			{
				return err;
			}

			int loop = 0;
			while ((err = m_cursor->get(&keyDbt, &valueDbt, GetNextFlag())) == 0 && ++loop < CURSOR_MAX_LOOP)
			{
				if (!CurrentPairCheckInRangeAndNeedContinue(keyDbt, valueDbt, err))
				{
					return err;
				}
			}
			return HandleErrNo(err);


		}

		InRangType InRange(KEY& keyValue, VALUE& valueValue)
		{
			if (CmpClass::Relative(keyValue, m_range.beginPair.first) == -1) return LessBeginKey;
			int k = CmpClass::Relative(keyValue, m_range.endPair.first);
			if (k == 1) return GreaterEndKey;
			if (k == -1) return YesInRange;
			if (k == 0)
			{
				if (CmpClass::Relative(valueValue, m_range.beginPair.second) == -1) return LessBeginValue;
				int v = CmpClass::Relative(valueValue, m_range.endPair.second);
				if (v == 0) return EqualEndValue;
				if (v == 1) return GreaterEndValue;
				return YesInRange;
			}


			return YesInRange;


		}

		bool DupAndSameKey()
		{
			if (!m_pDB->IsDup()) return false;

			if (CmpClass::Relative(m_range.beginPair.first, m_range.endPair.first) == 0) return true;
			return false;

		}

		u_int32_t GetNextFlag()
		{
			if (DupAndSameKey())
			{
				return DB_NEXT_DUP;
			}
			else
			{
				return DB_NEXT;
			}

		}

		u_int32_t FindFirstOneFlag()
		{
			if (DupAndSameKey())
			{
				// key必须完全匹配,value落在range中
				return DB_GET_BOTH_RANGE;
			}
			else
			{
				// key落在range中
				return DB_SET_RANGE;
			}
		}
	protected:
		FieldPairRange				m_range;




	};

}
