//
// Created by kingb on 2026/2/17.
//

#include "QueryThreadPoolStatus.h"
#include <Factory_Log.h>
#include <Factory_QDatabase.h>


void CQueryThreadPoolStatus::Go()
{
    int tastCount = MakeAndGet_QDatabase()->TaskCount();
    int idlCount = MakeAndGet_QDatabase()->IdlCount();
    Log_Print(LogLevel::Info, fmt::format("threadPool taskCount = {}, idlCount = {}", tastCount, idlCount));

}
