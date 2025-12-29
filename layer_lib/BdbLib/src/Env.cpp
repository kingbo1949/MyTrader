#include "Env.h"
#include "Global.h"
#include "MemFunsForBdb.h"

#include <boost/filesystem.hpp>
using namespace boost::filesystem;



CEnv::CEnv(const std::string& path, const std::string& dir, int sizeG, int sizeM, u_int32_t mutexInit, u_int32_t mutexIncrement)
:m_path(path), m_dir(dir), m_Env(DB_CXX_NO_EXCEPTIONS), m_sizeG(sizeG), m_sizeM(sizeM)
{

	u_int32_t envFlags =
		DB_CREATE		|	// Create the environment if it does not exist
		DB_INIT_CDB		|	// CDS 模式
		DB_INIT_MPOOL	|	// Initialize the memory pool (in-memory cache)
		DB_THREAD;			// Cause the environment to be free-threaded

	create_directory(dir.c_str());

	if (m_sizeG != 0 || m_sizeM != 0)
	{
		m_Env.set_cachesize(m_sizeG, m_sizeM*1014*1024, 0);
	}


	m_Env.mutex_set_increment(mutexIncrement);
	m_Env.mutex_set_init(mutexInit);
	m_Env.set_memory_max(0, 64000000);
	m_Env.set_alloc(CMemFunsForBdb::Malloc_BDB, CMemFunsForBdb::Realloc_BDB, CMemFunsForBdb::Free_BDB);

	// 删除之前数据库缓存数据
	DbEnv PreEnv(DB_CXX_NO_EXCEPTIONS);
	PreEnv.remove(m_dir.c_str(), DB_FORCE);

	int err = m_Env.open(m_dir.c_str(), envFlags, 0);
	printf("%s Env opened %d \n", m_dir.c_str(), err);


}

CEnv::~CEnv(void) 
{
	Destroy();
}

void CEnv::Destroy()
{
	Flush();
	// 写锁定
	RecMutex::Lock lock(m_mutex);
    
	m_dbFiles.clear();

	m_Env.close(DB_FORCESYNC);
	printf("database %s env closed! \n", m_dir.c_str());
}
void CEnv::Flush()
{
	std::vector<DbTablePtr> tables;
    {
        RecMutex::Lock lock(m_mutex);
        for (std::map<std::string, DbTablePtr>::const_iterator pos=m_dbFiles.begin();pos!=m_dbFiles.end();++pos)
        {
            tables.push_back(pos->second);
        }

    }
    for (std::vector<DbTablePtr>::const_iterator pos=tables.begin();pos!=tables.end();++pos)
    {
        int err = (*pos)->Flush();
        if (err)
        {
            printf("%s %s sync err %d \n", m_dir.c_str(), (*pos)->GetDbName().c_str(), err);
        }

    }
    return ;
}
std::string CEnv::GetDir()
{
    return m_dir;
}
void CEnv::SelfCheck()
{
	std::vector<DbTablePtr> dbTables;
	{
		RecMutex::Lock lock(m_mutex);
		for (std::map<std::string, DbTablePtr>::const_iterator pos=m_dbFiles.begin();pos!=m_dbFiles.end();++pos)
		{
			dbTables.push_back(pos->second);
		}

	}

	for (std::vector<DbTablePtr>::const_iterator pos=dbTables.begin();pos!=dbTables.end();++pos)
	{
		bool success = (*pos)->SelfCheck();
		if (!success)
		{
			printf("%s SelfCheck err \n", m_dir.c_str());
			// return ;
		}
		
	}

	printf("%s SelfCheck ok \n", m_dir.c_str());


}

DbEnv* CEnv::GetBdbEnv()
{
	return &m_Env;
}
void CEnv::AddDbFile(DbTablePtr pDb)
{
	RecMutex::Lock lock(m_mutex);
	std::string dbName = pDb->GetDbName();
	m_dbFiles[dbName] = pDb;

}
DbTablePtr CEnv::GetDbFile(const std::string& dbName)
{
	RecMutex::Lock lock(m_mutex);
	std::map<std::string, DbTablePtr>::const_iterator pos = m_dbFiles.find(dbName);
	if (pos == m_dbFiles.end()) return NULL;

	return pos->second;

}




