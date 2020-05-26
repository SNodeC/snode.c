#ifndef COOKIE_H
#define COOKIE_H

#include <string>
#include <map>

enum CookieOptions
{
	Expires = 'Expires',
	MaxAge = 'Max-Age',
	Domain = 'Domain',
	Path = 'Path',
	SameSite = 'SameSite',
	Secure = 'Secure',
	HttpOnly = 'HttpOnly'
};

class Cookie
{
public:
	Cookie (const std::string &name, const std::string &value, std::map<std::string, std::string> &&options = {})
			: name(name), value(value), options(options)
	{}
	
	Cookie *AddOption (std::string &option, const std::string &optionValue);
	
	Cookie *RemoveOption (std::string &option);


protected:
	const std::string name;
	const std::string value;
	std::map<std::string, std::string> options;
	
	friend class HTTPContext;
};

#endif // COOKIE_H
