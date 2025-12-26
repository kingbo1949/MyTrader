// QIBDatabase.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "Factory.h"
#include "testcase.h"
#include "Shell.h"

// 测试用例
void testBdb();

// 实时工作环境
int real_time_run(int argc, char* argv[])
{
	CShell shell;
	return shell.main(argc, argv, CONFIG_FILE.c_str());

}

int main(int argc, char* argv[])
{
	return real_time_run(argc, argv);
}

void testBdb()
{
	// 测试用例
	Env_IBPtr pEnv = MakeAndGet_Env();
	DbTable_TickHisPtr pTableTicks = pEnv->GetDB_TickHis();

	// 初始化数据并显示
	init_data(pTableTicks);

	// 测试全部查询
	//show_data_all(pTableTicks, "baba");
	//show_data_all(pTableTicks, "tencent");

	// 测试单个查询
	// show_data_one(pTableTicks, "baba", 4, 5);

	// 区域查询
	// show_data_range(pTableTicks, "tencent", 20, 26);

	// 从某一时点回溯
	// show_data_backward(pTableTicks, "baba", 6, 2);

	// 从某时点往后
	// show_data_forward(pTableTicks, "baba", 4, 6);

	// 从最近时间点回溯
	// show_data_recent(pTableTicks, "baba", 3);

}
