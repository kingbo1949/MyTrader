#pragma once

#include <RocksSerializer.h>
#include <QStruc.h>
#include <Ice/Ice.h>
#include <Ice/OutputStream.h>
#include <Ice/InputStream.h>

using namespace IBTrader;

// 为所有 Ice 数据结构定义统一的 Ice 序列化特化
ROCKS_ICE_SERIALIZER(IKLine)
ROCKS_ICE_SERIALIZER(ITick)
ROCKS_ICE_SERIALIZER(IMacdValue)
ROCKS_ICE_SERIALIZER(IAvgValue)
ROCKS_ICE_SERIALIZER(IDivTypeValue)
ROCKS_ICE_SERIALIZER(IAtrValue)
