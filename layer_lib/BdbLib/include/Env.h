#pragma once

#include "DbTable.h"
class CEnv : public IceUtil::Shared
{
public:
	CEnv(const std::string& path, const std::string& dir, int sizeG, int sizeM, u_int32_t mutexInit, u_int32_t mutexIncrement);
	virtual ~CEnv(void);

	std::string                             GetDir();

	DbEnv*									GetBdbEnv();


	void                                    Flush();
	void									SelfCheck();


protected:
	std::string								m_path;
	std::string								m_dir;
	DbEnv									m_Env;			// 环境指针

	int										m_sizeG;
	int										m_sizeM;

	std::map<std::string, DbTablePtr>		m_dbFiles;		// 文件名到文件实例的映射
	RecMutex								m_mutex;

	void									AddDbFile(DbTablePtr pDb);
	DbTablePtr								GetDbFile(const std::string& dbName);

	void									Destroy();

    



};
typedef IceUtil::Handle<CEnv> EnvPtr;
