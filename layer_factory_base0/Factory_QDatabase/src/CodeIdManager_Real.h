#pragma once
#include "CodeIdManager.h"
class CCodeIdManager_Real : public CCodeIdManager
{
public:
	CCodeIdManager_Real(SetupType setupType) ;
	virtual ~CCodeIdManager_Real() { ; };

	// 得到的所有codeId, 对组合品种不做切分
	virtual  IbContractPtrs				GetCodeId_ForAll()  override final;

	// 得到需要订阅行情的codeId, 对组合品种需要切分
	virtual  IbContractPtrs				GetCodeId_ForSubscribeQ()  override final;


protected:
	// 目标文件中所有的code, 包括单品种及混合品种
	std::set<CodeStr>					m_codes;

	// 从文件初始化 m_codes
	void								Init(SetupType setupType);


	// 把一个CodeStr插入到m_codes， 如果是混合品种，则需要拆分
	void								UpdateCode(const CodeStr& codeStr);


	// 打印m_codes内容
	void								ShowCodes();

	std::string							GetStr_MixCodes(const MixCodes& mixCodes);

	std::string							GetStr_Codes(const std::set<CodeStr>& codes);




};

