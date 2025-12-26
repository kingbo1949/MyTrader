#pragma once

#include <db_cxx.h>

#include <string>
#include <vector>
#include <map>

#include <IceUtil/IceUtil.h>
using namespace IceUtil;
#include "Db.h"

namespace Bdb
{
    class CDbTable : public IceUtil::Shared
    {
    public:
        CDbTable(DbEnv* env,const std::string& path, const std::string& dbName);

        virtual ~CDbTable(void);

        virtual	int		Flush() = 0;

        virtual	bool	SelfCheck() = 0;

        std::string	GetDbName()
        {
            return m_dbName;
        }


    protected:
        DbEnv*						m_env;				// 环境对象
        std::string					m_path;
        std::string					m_dbName;

    };
    typedef IceUtil::Handle<CDbTable> DbTablePtr;

    // 对于经常访问的小数据库表, 缓存数据库表的所有数据到map
    // 中间过程中之间操作内存数据
    // 每隔一段时间/退出的时候, 把所有内存数据同步保存的数据库

    template<typename KEY, typename VALUE>
    class CDbTableCache : public  CDbTable
    {
    protected:
        typedef CDb<KEY, VALUE> CDbKeyValue;
        typedef std::pair<KEY, VALUE>  FieldPair;
        typedef std::deque<FieldPair> FieldPairs;
        typedef std::map<KEY, VALUE> KeyValueMap;

        IceUtil::Handle<CDbKeyValue> m_pDb;
        KeyValueMap m_cacheDatas;
        IceUtil::Mutex m_mutex;

    public:
        CDbTableCache(DbEnv* env,const std::string& path, const std::string& dbName)
            : CDbTable(env, path, dbName)
        {

        }

        virtual ~CDbTableCache()
        {
            Flush();
        }

        // 加载所有旧数据到 m_cacheDatas
        virtual void LoadFromDB()
        {
            CDbJob_GetAll<KEY, VALUE> getAll(m_pDb);
            getAll.Execute();

            FieldPairs datas = getAll.GetResult();
            IceUtil::Mutex::Lock lock(m_mutex);

            unsigned int nSize = datas.size();

            for(unsigned int i = 0; i < nSize; ++i)
            {
                m_cacheDatas[datas[i].first] = datas[i].second;
            }

        }

        // 先清空数据库表, 再把所有缓存数据写到数据库
        virtual	int Flush()
        {
            m_pDb->RemoveAll();
            IceUtil::Mutex::Lock lock(m_mutex);

            typename std::map<KEY, VALUE>::iterator ie = m_cacheDatas.end();

            for(typename std::map<KEY, VALUE>::iterator it = m_cacheDatas.begin(); it != ie; ++it)
            {
                m_pDb->AddOne(it->first, it->second);
            }

            return m_pDb->Flush();
        }

        virtual	bool	SelfCheck()
        {
            return m_pDb->SelfCheck(m_dbName);
        }

        // 以下几个操作都是针对内存缓存数据

        void	AddOne(const KEY& key, const VALUE& value)
        {
            IceUtil::Mutex::Lock lock(m_mutex);
            m_cacheDatas[key] = value;
        }

        bool	GetOne(const KEY& key, VALUE& value)
        {
            IceUtil::Mutex::Lock lock(m_mutex);
            typename KeyValueMap::iterator it = m_cacheDatas.find(key);

            if (it  == m_cacheDatas.end())
            {
                return false;
            }

            value = it->second;
            return true;
        }

        void	RemoveOne(const KEY& key)
        {
            IceUtil::Mutex::Lock lock(m_mutex);
            typename KeyValueMap::iterator it = m_cacheDatas.find(key);

            if (it != m_cacheDatas.end())
            {
                m_cacheDatas.erase(it);
            }
        }

        void	GetAll(std::vector<VALUE>& values)
        {
            IceUtil::Mutex::Lock lock(m_mutex);
            values.clear();
            values.reserve(m_cacheDatas.size()+1);
            typename KeyValueMap::iterator ie = m_cacheDatas.end();

            for(typename KeyValueMap::iterator  it = m_cacheDatas.begin(); it != ie; ++it)
            {
                values.push_back(it->second);
            }
        }
    };
}
using namespace Bdb;
