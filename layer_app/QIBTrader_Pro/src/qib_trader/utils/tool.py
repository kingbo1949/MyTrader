from datetime import datetime


class TimeUtils:
    @staticmethod
    def ms_to_str(ms: int, include_ms: bool = True, fmt: str = "%Y-%m-%d %H:%M:%S") -> str:
        """
        将毫秒时间戳转换为日期字符串。

        :param ms: 毫秒时间戳 (int)
        :param include_ms: 是否包含毫秒部分
        :param fmt: 基础日期格式 (不含毫秒部分)
        :return: 格式化后的字符串
        """
        dt = datetime.fromtimestamp(ms / 1000.0)

        if not include_ms:
            return dt.strftime(fmt)

        # 获取基础格式字符串
        base_str = dt.strftime(fmt)
        # 计算毫秒部分：取余数并补齐 3 位零
        ms_part = f"{ms % 1000:03d}"

        return f"{base_str}.{ms_part}"

    @staticmethod
    def str_to_ms(date_str: str, fmt: str = "%Y-%m-%d %H:%M:%S") -> int:
        """
        将日期字符串转换为毫秒时间戳。
        缺省格式: "2026-03-07 05:05:48" 或 "2026-03-07 05:05:48.123"
        """
        try:
            # 1. 判断是否包含毫秒 (通过检查是否有小数点)
            if "." in date_str:
                # 使用 %f 解析，Python 的 %f 默认支持 1-6 位小数
                dt = datetime.strptime(date_str, f"{fmt}.%f")
            else:
                dt = datetime.strptime(date_str, fmt)

            # 2. 转换为秒级时间戳 (float) 并乘以 1000 得到毫秒
            return int(dt.timestamp() * 1000)

        except ValueError as e:
            raise ValueError(f"无法解析日期字符串 '{date_str}'，请确保格式与 '{fmt}' 匹配。") from e