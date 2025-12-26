#pragma once

#include "DbJob.h"

namespace Bdb
{
	template<typename KEY, typename VALUE>
	class CDbJob_GetForWard : public CDbJob<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_GetForWard<KEY, VALUE> >DbJob_GetForWardPtr;
		typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;
		typedef typename CDb<KEY, VALUE>::FieldPair FieldPair;
		typedef typename CDb<KEY, VALUE>::FieldPairs FieldPairs;
		typedef typename CDb<KEY, VALUE>::FieldPairRange FieldPairRange;
		typedef CDbJob<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;
	public:

		CDbJob_GetForWard(DbPtr pDb, const FieldPair& pair, Long count, bool forWholeFile)
			:BaseClass(pDb), m_pair(pair), m_count(count), m_forWholeFile(forWholeFile)
		{
			if (m_count <= 0)
			{
				m_count = LLONG_MAX;
			}

		}

		virtual ~CDbJob_GetForWard(void)
		{
		}

		void GetResult(std::vector<VALUE>& datas)
		{
			typename FieldPairs::const_iterator end = m_result.end();
			datas.reserve(m_result.size() + 1);

			for (typename FieldPairs::const_iterator pos = m_result.begin(); pos != end; ++pos)
			{
				datas.push_back(pos->second);
			}
		}

		void GetFieldPairs(FieldPairs& datas)
		{
			datas = m_result;
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
			// DB_GET_BOTH_RANGE:kye必须完全匹配，value作为range
			int err = m_cursor->get(&keyDbt, &valueDbt, DB_GET_BOTH_RANGE);
			if (err) return HandleErrNo(err);

			if (!CurrentPairCheckValidAndNeedContinue(keyDbt, valueDbt)) return 0;

			int loop = 0;
			while ((err = m_cursor->get(&keyDbt, &valueDbt, GetNextFlag())) == 0 && ++loop < CURSOR_MAX_LOOP)
			{
				if (!CurrentPairCheckValidAndNeedContinue(keyDbt, valueDbt)) return 0;
			}
			return HandleErrNo(err);


		}
		bool CurrentPairCheckValidAndNeedContinue(const Dbt& key, const Dbt& value)
		{
			KEY keyValue;
			VALUE valueValue;
			UnMarshalObject<KEY>(key, keyValue);
			UnMarshalObject<VALUE>(value, valueValue);
			FieldPair currentPair = std::make_pair(keyValue, valueValue);

			m_result.push_back(currentPair);

			if (m_result.size() >= size_t(m_count)) return false;

			return true;
		}
		u_int32_t GetNextFlag()
		{
			if (m_forWholeFile)
			{
				return DB_NEXT;
			}
			else
			{
				return DB_NEXT_DUP;
			}

		}
	protected:
		FieldPair					m_pair;
		Long						m_count;
		bool						m_forWholeFile;
		FieldPairs					m_result;
	};

}
