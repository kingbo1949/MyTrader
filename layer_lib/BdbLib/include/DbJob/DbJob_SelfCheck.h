#pragma once

#include "DbJob.h"
#include "Global.h"

namespace Bdb
{
	template<typename KEY, typename VALUE>
	class CDbJob_SelfCheck : public CDbJob<KEY, VALUE>
	{
	public:
		typedef IceUtil::Handle<CDbJob_SelfCheck<KEY, VALUE> >DbJob_SelfCheckPtr;
		typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;
		typedef typename CDb<KEY, VALUE>::FieldPair FieldPair;
		typedef CDbJob<KEY, VALUE> BaseClass;
		using BaseClass::m_pDB;
		using BaseClass::HandleErrNo;
		using BaseClass::m_cursor;

		struct SelfCheckResult
		{
			// key中内容是否有效,如果有效是因为数据库内存写坏了，这种错误数据库是无法
			// 检测的.返回值的检查不仅需要检查数据返回值,也需要检查keyValid
			bool					keyValid;
			KEY						key;
			int						recordCount;	// 扫描过的记录数
			int						errCode;
			SelfCheckResult()
			{
				keyValid = false;
				recordCount = 0;
				errCode = 0;
			}

		};




	public:

		CDbJob_SelfCheck(DbPtr pDb, const std::string& tableName)
			:BaseClass(pDb), m_tabelName(tableName), m_sepcMilliSecPrint(10000), m_sepcMilliSecProcess(90000)
		{
		}

		virtual ~CDbJob_SelfCheck(void)
		{
		}
		std::string				GetTableName()
		{
			return m_tabelName;
		}
		SelfCheckResult			GetResult()
		{
			return m_result;
		}

		std::string				GetLogFileName(bool isSuccess)
		{
			// 返回selfcheck_YYYYMMDD.log
			std::string timeStr = CGlobal::GetTimeStr(time(NULL));
			timeStr = timeStr.substr(0, 8);

			if (isSuccess)
			{
				std::string back = "selfcheck_" + timeStr + ".log";
				return back;
			}
			else
			{
				std::string back = "selfcheckErr_" + timeStr + ".log";
				return back;

			}


		}

	protected:
		std::string			m_tabelName;

		const int			m_sepcMilliSecPrint;			// 打印时间间隔
		const int			m_sepcMilliSecProcess;			// 测试进度时间间隔

		SelfCheckResult		m_result;



		// 打印进度,并返回进度乘10000的整数
		int				PrintProcess(Dbt* pkey, int& process)
		{
			DB_KEY_RANGE temRange;
			m_result.errCode = m_pDB->GetDb()->key_range(NULL, pkey, &temRange, 0);
			if (m_result.errCode) return m_result.errCode;

			process = int(temRange.less * 10000); // 进度整数

			char buff[500];
			memset(buff, 0, 500);
			CGlobal::SafePrintf(buff, 500, "recordNum = %d ,%5.3f%%", m_result.recordCount, float(temRange.less * 100));

			CLog::Instance()->PrintLog(GetLogFileName(true), "%s \n", buff);


			return 0;

		}

		virtual int		Setup_Cursor()
		{
			m_result.errCode = m_pDB->GetDb()->cursor(NULL, &m_cursor, 0);
			return m_result.errCode;
		}

		virtual int		RealExecute()
		{

			Dbt keyDbt, valueDbt;

			m_result.errCode = 0;

			int lastProcess = 0;
			int thisProcess = 0;

			IceUtil::Int64 beginPrintTime = Time::now().toMilliSeconds();
			IceUtil::Int64 beginProcessTime = beginPrintTime;

			while ((m_result.errCode = m_cursor->get(&keyDbt, &valueDbt, DB_NEXT)) == 0)
			{
				UnMarshalObject<KEY>(keyDbt, m_result.key);
				m_result.recordCount++;

				if (Time::now().toMilliSeconds() - beginPrintTime > m_sepcMilliSecPrint)
				{
					// 打印
					m_result.errCode = PrintProcess(&keyDbt, thisProcess);
					if (m_result.errCode)
					{
						CLog::Instance()->PrintLog(GetLogFileName(true), "%s PrintProcess return err=%d \n",
							m_tabelName.c_str(), m_result.errCode);
					}
					beginPrintTime = Time::now().toMilliSeconds();

				}
				if (Time::now().toMilliSeconds() - beginProcessTime > m_sepcMilliSecProcess)
				{
					// 测试进度
					if (thisProcess != lastProcess)
					{
						lastProcess = thisProcess;
						beginProcessTime = Time::now().toMilliSeconds();
					}
					else
					{
						// 进度没有进展 内存错误
						m_result.keyValid = true;
						m_result.errCode = 0;
						break;

					}


				}
				if (m_result.errCode != 0) break;
			}

			m_result.errCode = HandleErrNo(m_result.errCode);
			return m_result.errCode;


		}



	};

}

