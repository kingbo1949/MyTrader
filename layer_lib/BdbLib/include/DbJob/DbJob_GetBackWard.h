#pragma once

#include "DbJob.h"
#include <climits>
#include "Field.h"
namespace Bdb
{
	class CCompare;
	template<typename KEY, typename VALUE, class CmpClass = CCompare>
	class CDbJob_GetBackWard : public CDbJob<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_GetBackWard<KEY, VALUE> >DbJob_GetBackWardPtr;
		typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;
		typedef typename CDb<KEY, VALUE>::FieldPair FieldPair;
		typedef typename CDb<KEY, VALUE>::FieldPairs FieldPairs;
		typedef typename CDb<KEY, VALUE>::FieldPairRange FieldPairRange;
		typedef CDbJob<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;
	public:

		CDbJob_GetBackWard(DbPtr pDb, const FieldPair& pair, const FieldPair& lastPair, Long count)
			:BaseClass(pDb), m_pair(pair), m_lastPair(lastPair), m_count(count)
		{
			if (m_count <= 0)
			{
				m_count = LLONG_MAX;
			}
			int ret = CmpClass::Relative(m_pair.second, m_lastPair.second);
			if (ret == 0 || ret == 1)
			{
				m_pair = m_lastPair;
			}

		}

		virtual ~CDbJob_GetBackWard(void)
		{
		}

		void GetResult(std::vector<VALUE>& datas)
		{
			if (m_result.size() >= 2 && m_result[0].second.time >= m_result[1].second.time)
			{
				reverse(m_result.begin(), m_result.end());
			}

			typename FieldPairs::const_iterator end = m_result.end();
			datas.reserve(m_result.size() + 1);

			for (typename FieldPairs::const_iterator pos = m_result.begin(); pos != end; ++pos)
			{
				datas.push_back(pos->second);
			}
		}
	protected:

		virtual int		Setup_Cursor()
		{
			int err = m_pDB->GetDb()->cursor(NULL, &m_cursor, 0);
			return err;
		}

		virtual int		RealExecute()
		{
			Ice::ByteSeq keybuff;
			Dbt keyDbt;
			MarshalObject(keybuff, m_pair.first, keyDbt);

			Ice::ByteSeq valuebuff;
			Dbt	valueDbt;
			MarshalObject(valuebuff, m_pair.second, valueDbt);

			// 查找第一个位置
			int err = m_cursor->get(&keyDbt, &valueDbt, DB_GET_BOTH_RANGE);
			if (err) return HandleErrNo(err);

			if (!CurrentPairCheckValidAndNeedContinue(keyDbt, valueDbt)) return 0;

			int loop = 0;
			while ((err = m_cursor->get(&keyDbt, &valueDbt, DB_PREV_DUP)) == 0 && ++loop < CURSOR_MAX_LOOP)
			{
				if (!CurrentPairCheckValidAndNeedContinue(keyDbt, valueDbt)) return 0;
			}
			return HandleErrNo(err);


		}
		bool CurrentPairCheckValidAndNeedContinue(const Dbt& keyDbt, const Dbt& valueDbt)
		{
			KEY keyValue;
			VALUE valueValue;
			UnMarshalObject<KEY>(keyDbt, keyValue);
			UnMarshalObject<VALUE>(valueDbt, valueValue);


			int ret = CmpClass::Relative(valueValue, m_pair.second);
			if (ret < 0 || ret == 0)
			{
				m_result.push_back(std::make_pair(keyValue, valueValue));
			}
			if (m_result.size() >= size_t(m_count)) return false;

			return true;
		}


	protected:
		FieldPair				m_pair;			// 定位记录
		FieldPair				m_lastPair;		// 数据库中最后一条记录
		Long					m_count;
		FieldPairs				m_result;



	};

}
