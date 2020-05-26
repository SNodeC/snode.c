#include "Cookie.h"

Cookie *Cookie::AddOption (std::string &option, const std::string &optionValue)
{
	options.insert({option, optionValue});
	return this;
}

Cookie *Cookie::RemoveOption (std::string &option)
{
	options.erase(option);
	return this;
}