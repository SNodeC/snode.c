#ifndef COOKIE_H
#define COOKIE_H

#include <string>
#include <map>

class Cookie
{
public:
	Cookie (const std::string &name, const std::string &value, const std::map<std::string, std::string> &options = {})
			: name(name), value(value), options(options)
	{}
	
	Cookie *AddOption (const std::string &option, const std::string &optionValue);
	
	Cookie *RemoveOption (const std::string &option);


protected:
	const std::string name;
	const std::string value;
	std::map<std::string, std::string> options;
	
	friend class HTTPContext;
};

#endif // COOKIE_H
