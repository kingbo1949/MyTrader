#pragma once
#include "DbJob.h"
#include "Field.h"
namespace Bdb
{
	template<typename KEY, typename VALUE>
	class CDbJob_GetOne : public CDbJob<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_GetOne<KEY, VALUE> >DbJob_GetOnePtr;
		typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;
		typedef typename CDb<KEY, VALUE>::FieldPair FieldPair;
		typedef CDbJob<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;
	public:

		CDbJob_GetOne(DbPtr pDb, const KEY& _key, VALUE& _value)
			:BaseClass(pDb), m_validValue(false), m_key(_key), m_value(_value)
		{
		}

		virtual ~CDbJob_GetOne(void)
		{
		}

		bool			GetResult()
		{
			return m_validValue;
		}

	protected:
		bool                        m_validValue;
		const KEY& m_key;
		VALUE& m_value;

		virtual int		Setup_Cursor()
		{
			int err = m_pDB->GetDb()->cursor(NULL, &m_cursor, 0);
			return err;
		}

		virtual int		RealExecute()
		{
			Ice::ByteSeq keybuff;
			Dbt keyDbt;
			MarshalObject(keybuff, m_key, keyDbt);

			Ice::ByteSeq valuebuff;
			Dbt	valueDbt;
			MarshalObject(valuebuff, m_value, valueDbt);

			// 查找位置
			u_int32_t flag = m_pDB->MakeFlagForGetOneJob();
			int err = m_cursor->get(&keyDbt, &valueDbt, flag);
			if (err) return HandleErrNo(err);

			m_validValue = true;
			UnMarshalObject<VALUE>(valueDbt, m_value);

			return 0;
		}


	};



}

