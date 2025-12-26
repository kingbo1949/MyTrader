#pragma once
#include "DbJob_Range.h"

namespace Bdb
{
	template<typename KEY, typename VALUE>
	class CDbJob_Range_Get : public CDbJob_Range<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_Range_Get<KEY, VALUE> > DbJob_Range_GetPtr;
		typedef CDbJob_Range<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;
	public:

		CDbJob_Range_Get(typename BaseClass::DbPtr pDb, const typename BaseClass::FieldPairRange& range)
			:BaseClass(pDb, range)
		{
		}

		virtual ~CDbJob_Range_Get(void)
		{
		}



	protected:
		virtual void Add(KEY& keyValue, VALUE& valueValue) = 0;

		virtual int		Setup_Cursor()
		{
			int err = m_pDB->GetDb()->cursor(NULL, &m_cursor, 0);
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
				Add(keyValue, valueValue);

			}
			if (inRangeType == GreaterEndKey || inRangeType == GreaterEndValue || inRangeType == EqualEndValue)
			{
				return false;
			}
			return true;


		}



	};

	template<typename KEY, typename VALUE>
	class CDbJob_Range_Get_Values : public CDbJob_Range_Get<KEY, VALUE>
	{

	public:
		typedef std::vector<VALUE> Values;
		typedef CDbJob_Range_Get<KEY, VALUE> BaseClass;
		typedef IceUtil::Handle<CDbJob_Range_Get_Values<KEY, VALUE> > DbJob_Range_Get_ValuesPtr;

		CDbJob_Range_Get_Values(typename BaseClass::DbPtr pDb, const typename BaseClass::FieldPairRange& range, Values& result)
			: BaseClass(pDb, range)
			, m_result(result)
		{

		}

		virtual void Add(KEY& keyValue, VALUE& valueValue)
		{
			m_result.push_back(valueValue);
		}

	protected:
		Values& m_result;

	};

	template<typename KEY, typename VALUE>
	class CDbJob_Range_Get_FieldPairs : public CDbJob_Range_Get<KEY, VALUE>
	{
		typedef CDbJob_Range_Get<KEY, VALUE> BaseClass;

	public:
		typedef IceUtil::Handle<CDbJob_Range_Get_FieldPairs<KEY, VALUE> > DbJob_Range_Get_FieldPairsPtr;
		CDbJob_Range_Get_FieldPairs(typename BaseClass::DbPtr pDb, const typename BaseClass::FieldPairRange& range, typename BaseClass::FieldPairs& result)
			: BaseClass(pDb, range)
			, m_result(result)
		{

		}

	protected:
		virtual void Add(KEY& keyValue, VALUE& valueValue)
		{
			m_result.push_back(make_pair(keyValue, valueValue));
		}

	protected:
		typename BaseClass::FieldPairs& m_result;
	};

}
