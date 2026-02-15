#pragma once

#include <cstring>
#include <string>
#include <type_traits>
#include <rocksdb/slice.h>

/// 序列化策略 trait（默认实现：POD 类型直接 memcpy）
/// 对于 POD 结构体（如 KLine 的 OHLCV），零开销直接内存拷贝
/// 非 POD 类型需要提供特化版本
///
/// 特化示例（Ice 类型）：
///   template<>
///   struct RocksSerializer<MyIceStruct> {
///       static std::string Serialize(const MyIceStruct& obj) { ... }
///       static MyIceStruct Deserialize(const rocksdb::Slice& data) { ... }
///   };
template<typename T>
struct RocksSerializer {
    static_assert(std::is_trivially_copyable_v<T>,
        "Default RocksSerializer requires trivially copyable type. "
        "For non-POD types, provide a template specialization.");

    /// 将 POD 对象序列化为 std::string（二进制拷贝）
    static std::string Serialize(const T& obj)
    {
        return std::string(reinterpret_cast<const char*>(&obj), sizeof(T));
    }

    /// 从 rocksdb::Slice 反序列化为 POD 对象
    static T Deserialize(const rocksdb::Slice& data)
    {
        T obj;
        std::memcpy(&obj, data.data(), sizeof(T));
        return obj;
    }
};

/// Ice 类型序列化辅助宏
/// 用法：在业务层头文件中对 Ice 生成的结构体使用
///   ROCKS_ICE_SERIALIZER(MyModule::KLineData)
/// 展开后生成对应的 RocksSerializer 特化
#define ROCKS_ICE_SERIALIZER(IceType)                                       \
template<>                                                                  \
struct RocksSerializer<IceType> {                                           \
    static std::string Serialize(const IceType& obj)                        \
    {                                                                       \
        Ice::OutputStream out;                                              \
        out.write(obj);                                                     \
        Ice::ByteSeq data;                                                  \
        out.finished(data);                                                 \
        return std::string(reinterpret_cast<const char*>(data.data()),      \
                           data.size());                                    \
    }                                                                       \
    static IceType Deserialize(const rocksdb::Slice& data)                  \
    {                                                                       \
        Ice::ByteSeq bytes(                                                 \
            reinterpret_cast<const uint8_t*>(data.data()),                  \
            reinterpret_cast<const uint8_t*>(data.data()) + data.size());   \
        Ice::InputStream in(bytes);                                         \
        IceType obj;                                                        \
        in.read(obj);                                                       \
        return obj;                                                         \
    }                                                                       \
};
