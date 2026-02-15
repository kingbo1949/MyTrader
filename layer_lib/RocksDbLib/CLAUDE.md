# RocksDbLib 开发规则

## 架构规则
- RocksDbLib 是 BdbLib 的 RocksDB 替代品，提供相同层级的模板化数据库封装
- 构建为静态库（STATIC），与 BdbLib 保持一致
- 公共头文件放在 include/ 目录，对外暴露 PUBLIC include path

## 编码规则
- 遵循项目 C++17 标准，不使用命名空间
- 类命名沿用 BdbLib 的 C 前缀风格（如 CRocksEnv, CRocksDb）
- 序列化优先继续使用 Ice 的 marshal/unmarshal 机制（通过 ZeroIce::all），如果更好的序列化方案可以提出来讨论
- RocksDbLib 自身无需加锁（见线程安全规则）

## 依赖规则
- PUBLIC 依赖：RocksDB::rocksdb, ZeroIce::all
- PRIVATE 依赖：ToolLib, Boost::all
- 不直接依赖 BdbLib，两者可并存

## 添加文件和目录规则
- 每次添加文件或者目录，不能超过2个，并且要讲解他们添加的用途
- 不要一次就生成整个类的.h文件和.cpp文件，而是要每次添加一个函数声明到头文件，讲解清楚函数用途和每个参数的意义之后获得允许，再在.cpp文件中添加实现部分

## 数据库规则
- 每张表的key必须是固定长度，比如Kline表的key为char[64] + time_t，固定72字节
- 列族按"数据类型_时间周期"命名，如 KLINE_M5、MACD_M5、ATR_D1
- 同一数据类型的不同周期是不同列族，同一周期的不同数据类型也是不同列族
- 业务层启动时通过 GetOrCreateColumnFamily 获取列族 handle 并缓存，运行时直接用 handle 指针操作，避免字符串查找开销
- 不必参照BdbLib的实现，而是按照RocksDb的标准实现方式来设计、实现并导出接口

## 线程安全规则
- 列族仅在启动阶段创建（单线程），运行阶段不再创建或删除列族
- 运行阶段多线程并发读写是安全的，RocksDB 引擎保证并发安全，RocksDbLib 无需额外加锁
- CRocksWriteBatch 实例不跨线程共享，每个线程持有自己的 batch

## 跨平台规则
- 代码必须在 Windows (x86_64) 和 macOS (x86_64/ARM64) 上正确执行
- Key 中的数值类型（如 time_t）必须以大端序（Big-Endian）存储，确保 RocksDB 字节序比较结果与数值大小一致
- 不依赖平台特定的字节序，使用显式的字节序转换函数
