#pragma once
#include <base_struc.h>
class CSetupEnv
{
public:
	CSetupEnv() { ; };
	virtual ~CSetupEnv() { ; };

	void				Init();

	// 启动远程API
	void				RunIBApi();

	void				Release();


protected:

	void				Init_Factory_Log();
	void				Init_Factory_Global();
	void				Init_Factory_CodeIdHashEnv();
	void				Init_Factory_QDatabase();



};

