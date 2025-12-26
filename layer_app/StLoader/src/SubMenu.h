#pragma once

#include <string>
#include <memory>
class CSubMenu
{
public:
	CSubMenu() { ; };
	virtual ~CSubMenu() { ; };

	void				Go() ;

private:
	virtual void		ShowMenu() = 0;
	virtual bool		HandleReceive(const std::string& cmd) = 0;


};
typedef std::shared_ptr<CSubMenu> SubMenuPtr;
