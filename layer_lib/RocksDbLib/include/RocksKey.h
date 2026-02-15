#pragma once

#include <cstring>
#include <ctime>
#include <cstdint>
#include <string>
#include <rocksdb/slice.h>

#ifdef _WIN32
#include <stdlib.h>   // _byteswap_uint64
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#else
#include <endian.h>   // htobe64 / be64toh
#endif

/// 本机字节序 → 大端序，确保 RocksDB 字节序比较与数值顺序一致
inline uint64_t ToBigEndian(uint64_t val)
{
#ifdef _WIN32
    return _byteswap_uint64(val);
#elif defined(__APPLE__)
    return OSSwapHostToBigInt64(val);
#else
    return htobe64(val);
#endif
}

/// 大端序 → 本机字节序
inline uint64_t FromBigEndian(uint64_t val)
{
#ifdef _WIN32
    return _byteswap_uint64(val);
#elif defined(__APPLE__)
    return OSSwapBigToHostInt64(val);
#else
    return be64toh(val);
#endif
}

/// 合约+时间戳 复合 Key，固定 72 字节
/// 用于 KLine、MACD、ATR 等所有按"合约+时间"索引的列族
///
/// 内存布局（紧凑排列，无填充）：
///   [0,  63] char[64]  合约名，不足部分用 '\0' 填充
///   [64, 71] uint64_t  K线时间戳，大端序存储，确保字节序比较正确
///
/// RocksDB 默认按字节序比较 key，大端序保证：
///   - 同一合约的数据按时间正序排列
///   - 可用前 64 字节做前缀扫描，获取某合约的全部数据
///   - Windows 和 macOS 上行为一致
#pragma pack(push, 1)
struct CRocksKey {
    char symbol[64];        ///< 合约名称，如 "TSLA"、"ES"，不足 64 字节用 '\0' 填充
    uint64_t timestampBE;   ///< K线时间戳，大端序存储（不要直接读写，用构造函数和 GetTimestamp）

    /// 默认构造：全部清零
    CRocksKey()
    {
        std::memset(symbol, 0, sizeof(symbol));
        timestampBE = 0;
    }

    /// 构造函数
    /// @param sym 合约名称字符串，超过 63 字节会截断（保留末尾 '\0'）
    /// @param ts  K线时间戳（本机字节序），内部自动转为大端序存储
    CRocksKey(const std::string& sym, time_t ts)
    {
        std::memset(symbol, 0, sizeof(symbol));
        std::strncpy(symbol, sym.c_str(), sizeof(symbol) - 1);
        timestampBE = ToBigEndian(static_cast<uint64_t>(ts));
    }

    /// 获取时间戳（从大端序还原为本机字节序）
    /// @return time_t 时间戳
    time_t GetTimestamp() const
    {
        return static_cast<time_t>(FromBigEndian(timestampBE));
    }

    /// 转换为 rocksdb::Slice，用于 Put/Get/Delete 操作
    /// @return 指向本结构体内存的 Slice，生命周期与本对象一致
    rocksdb::Slice ToSlice() const
    {
        return rocksdb::Slice(reinterpret_cast<const char*>(this), sizeof(CRocksKey));
    }

    /// 从 rocksdb::Slice 解析出 CRocksKey
    /// @param slice 必须恰好 72 字节（sizeof(CRocksKey)）
    /// @return 解析后的 CRocksKey 副本
    static CRocksKey FromSlice(const rocksdb::Slice& slice)
    {
        CRocksKey key;
        if (slice.size() == sizeof(CRocksKey)) {
            std::memcpy(&key, slice.data(), sizeof(CRocksKey));
        }
        return key;
    }

    /// 提取合约名称为 std::string
    std::string GetSymbol() const
    {
        return std::string(symbol);
    }

    /// 生成仅包含合约名的前缀 Slice，用于前缀扫描
    /// @return 前 64 字节的 Slice
    rocksdb::Slice SymbolPrefix() const
    {
        return rocksdb::Slice(symbol, sizeof(symbol));
    }
};
#pragma pack(pop)

static_assert(sizeof(CRocksKey) == 72, "CRocksKey must be exactly 72 bytes");
