#include "DbTable.h"

CDbTable::CDbTable(DbEnv* env,const std::string& path, const std::string& dbName)
:m_env(env), m_path(path), m_dbName(dbName)
{
}

CDbTable::~CDbTable(void)
{
}
