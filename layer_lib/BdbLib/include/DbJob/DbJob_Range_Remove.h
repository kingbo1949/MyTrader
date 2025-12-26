#pragma once

#include "DbJob_Range.h"

namespace Bdb
{
	template<typename KEY, typename VALUE>
	class CDbJob_Range_Remove : public CDbJob_Range<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_Range_Remove<KEY, VALUE> >DbJob_Range_RemovePtr;
		typedef CDbJob_Range<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;


	public:

		CDbJob_Range_Remove(typename BaseClass::DbPtr pDb, const typename BaseClass::FieldPairRange& range)
			:BaseClass(pDb, range)
		{
		}

		virtual ~CDbJob_Range_Remove(void)
		{
		}
	protected:
		virtual int		Setup_Cursor()
		{
			int err = m_pDB->GetDb()->cursor(NULL, &m_cursor, DB_WRITECURSOR);
			return err;
		}

		virtual bool CurrentPairCheckInRangeAndNeedContinue(const Dbt& key, const Dbt& value, int& err)
		{
			err = 0;
			KEY keyValue;
			VALUE valueValue;
			UnMarshalObject<KEY>(key, keyValue);
			UnMarshalObject<VALUE>(value, valueValue);

			InRangType inRangeType = CDbJob_Range<KEY, VALUE>::InRange(keyValue, valueValue);
			if (inRangeType == YesInRange)
			{
				err = m_cursor->del(0);

				if (err) return false;
			}
			if (inRangeType == GreaterEndKey || inRangeType == GreaterEndValue || inRangeType == EqualEndValue)
			{
				return false;
			}
			return true;
		}

	};

}
