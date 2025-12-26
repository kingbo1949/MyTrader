#pragma once

#include "Db.h"
#include <typeinfo>
#include <string>
#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include "Global.h"
using namespace IceUtil;

using namespace Ice;
namespace Bdb
{
    enum InRangType
    {
        YesInRange,					// 在区间内
        LessBeginKey,				// 比beginKey要小
        GreaterEndKey,				// 比endKey要大
        LessBeginValue,				// key适合,但是value要比beginValue小
        EqualEndValue,				// key适合,但是value等于endValue
        GreaterEndValue				// key适合,但是value大于endValue
    };

	// 为防止游标失效出现死循环, 定义最大循环次数,限制获取数据总量
	const int CURSOR_MAX_LOOP = 30000000;

    template<typename KEY, typename VALUE> class CDb;


    template<typename KEY, typename VALUE>
    class CDbJob : public IceUtil::Shared
    {
    public:
        typedef IceUtil::Handle<CDbJob<KEY, VALUE> >DbJobPtr;
        typedef typename CDb<KEY, VALUE>::DbPtr DbPtr;


    protected:
        DbPtr				m_pDB;
        Dbc*				m_cursor;						// 光标指针

        virtual int			Setup_Cursor() = 0;				// 建立光标
        virtual int			RealExecute() = 0;				// 处理主体

        int					HandleErrNo(int souceErr)
        {
            if (souceErr == 0 || souceErr == DB_NOTFOUND)
            {
                // 游标已经越过表尾，指向表尾，正常退出
                return 0;
            }else
            {
                // 非正常退出
                MakeErrStr("HandleErrNo", souceErr);
                return souceErr;
            }
        }

        // 制作err字符串
        void				MakeErrStr(const std::string& reason, int err)
        {
            char buff[1025] = {0};
            CGlobal::SafePrintf(buff, 1024, "%s %s %s err=%d",
                m_pDB->GetDbName().c_str(), typeid(*this).name(), reason.c_str(), err);


            CLog::Instance()->PrintLog("MakeErrStr.log", "%s %s %s err=%d",
                m_pDB->GetDbName().c_str(), typeid(*this).name(), reason.c_str(), err);


            printf("%s \n", buff);


        }



    public:
        CDbJob(DbPtr pDb)
            :m_pDB(pDb), m_cursor(NULL)
        {

        }
        virtual ~CDbJob(void){;}



        bool				Execute()
        {
            // 建立光标
            int err = Setup_Cursor();
            if (err)
            {
                MakeErrStr("Setup_Cursor failed!", err);
                return false;
            }

            // 移动光标并且完成操作
            err = RealExecute();
            // 关闭光标
            if (m_cursor)
            {
                m_cursor->close();
            }

            if (err)
            {
                // 错误字符串
                MakeErrStr("RealExecute failed!", err);
                return false;
            }

            return true;
        }
    };


}
using namespace Bdb;

