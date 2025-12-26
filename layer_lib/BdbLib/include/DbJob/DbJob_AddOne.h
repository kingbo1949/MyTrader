#pragma once

#include "DbJob.h"
#include "Field.h"
namespace Bdb
{
	template<typename T>
	class CField;

	template<typename KEY, typename VALUE>
	class CDbJob_AddOne : public CDbJob<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_AddOne<KEY, VALUE> >DbJob_AddOnePtr;
		typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;
		typedef typename CDb<KEY, VALUE>::FieldPair FieldPair;

		typedef CDbJob<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::m_cursor;
		using BaseClass::HandleErrNo;
	public:
		CDbJob_AddOne(typename BaseClass::DbPtr pDb, const KEY& _first, const VALUE& _second)
			:BaseClass(pDb), first(_first), second(_second)
		{

		}
		virtual ~CDbJob_AddOne(void) {}

	private:
		const KEY& first;
		const VALUE& second;


		virtual int					Setup_Cursor() { return 0; }
		virtual int					RealExecute()
		{
			Ice::ByteSeq keybuff;
			Dbt keyDbt;
			MarshalObject(keybuff, first, keyDbt);

			Ice::ByteSeq valuebuff;
			Dbt	valueDbt;
			MarshalObject(valuebuff, second, valueDbt);


			if (keyDbt.get_size() == 0 || valueDbt.get_size() == 0)
			{
				return 0;
			}

			u_int32_t flags = m_pDB->MakeFlagForAddOneJob(0);
			int err = m_pDB->GetDb()->put(NULL, &keyDbt, &valueDbt, flags);

			return err;

		}
	};


}

