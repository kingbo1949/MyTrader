#pragma once
#include <base_struc.h>
#include <EClientSocket.h>
#include <EReader.h>
#include <EReaderOSSignal.h>

// ib的守护线程，行情交易通用，不断地响应刷新回调结果
class CCmd_IBDaemon
{
public:
	CCmd_IBDaemon(EClientSocket* const pApi, EReader* pReader, EReaderOSSignal* pOsSignal);
	virtual ~CCmd_IBDaemon() { ; };

	void					operator()();

	std::string					GetThreadName() { return "CCmdQGernerator_RealTime"; };
	 
protected:

	EClientSocket* const	m_pApi;
	EReader*				m_pReader;
	EReaderOSSignal*		m_pOsSignal;

	void					ProcessMessages();

};
typedef std::shared_ptr<CCmd_IBDaemon> Cmd_IBDaemonPtr;


