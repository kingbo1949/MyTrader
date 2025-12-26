#pragma once

#include <db_cxx.h>
#include <Ice/Ice.h>
#include <Ice/OutputStream.h>
#include <Ice/InputStream.h>



namespace Bdb
{
	// 从Dbt缓冲解编一个对象
	template<typename T>
	void UnMarshalObject(const Dbt& dbt, T& back)
	{
		Ice::ByteSeq data(static_cast<const unsigned char*>(dbt.get_data()),
			static_cast<const unsigned char*>(dbt.get_data()) + dbt.get_size());
		Ice::InputStream inStream(data);
		inStream.read(back);
		return;
	}

	// 整编一个对象到dbt
	template<typename T>
	void MarshalObject(Ice::ByteSeq& data, const T& value, Dbt& dbt)
	{
		data.reserve(1024);

		Ice::OutputStream out;
		out.write(value);

		// 获取序列化后的 ByteSeq 数据
		out.finished(data);

		dbt.set_data(data.data());      // 设置指针
		dbt.set_size(u_int32_t(data.size()));      // 设置大小
		dbt.set_ulen(u_int32_t(data.capacity()));
		dbt.set_flags(DB_DBT_USERMEM);
		return;
	}

}

