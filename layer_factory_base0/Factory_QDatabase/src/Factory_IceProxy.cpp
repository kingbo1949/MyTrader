#include "pch.h"
#include "Factory_IceProxy.h"
#include "IceProxy_Real.h"

IceProxyPtr g_pIceProxy = nullptr;

IceProxyPtr MakeAndGet_IceProxy()
{
	if (!g_pIceProxy)
	{
		g_pIceProxy = std::make_shared<CIceProxy_Real>();
	}
	return g_pIceProxy;
}
