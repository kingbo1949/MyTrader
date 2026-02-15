// QIBDatabase.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "Shell.h"

int main(int argc, char* argv[])
{
	CShell shell;
	return shell.main(argc, argv, CONFIG_FILE.c_str());
}
