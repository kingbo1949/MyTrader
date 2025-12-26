// QuoteGenerator.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <base_struc.h>

#include <signal.h>
#include <thread>
#include <Factory_Setup.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_UnifyInterface.h>


void sighandler(int signum);


Thread_Status g_main_status = Thread_Status::Running;

int main()
{
	signal(SIGINT, sighandler); 

#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8); // 设置控制台为 UTF-8, windows的控制台默认是GBK
#endif


	Make_Setup(SetupType::QGenerator);
	Get_Setup()->Open();

	// 设定启动时间
	SetSetupTime(Get_CurrentTime()->GetCurrentTime_second());

	while (g_main_status == Thread_Status::Running)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	// 准备退出
	printf("will close ...... \n");
	Get_Setup()->Close();

	printf("close success \n");

	int threadCount = MakeAndGet_MyThreadPool()->idlCount();
	printf("mythreadpool count is %d \n", threadCount);

	return 0;
}

void sighandler(int signum)
{
	if (signum != SIGINT) return;

	// ctrl-C,关闭程序
	g_main_status = Thread_Status::Stop;
	return;
}


