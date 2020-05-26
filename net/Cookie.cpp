#include "Cookie.h"

Cookie *Cookie::AddOption (const std::string &option, const std::string &optionValue)
{
	options.insert({option, optionValue});
	return this;
}

Cookie *Cookie::RemoveOption (const std::string &option)
{
	options.erase(option);
	return this;
}
