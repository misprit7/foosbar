#pragma once
#include "pugixml.hpp"

class ErrCodeStr
{
private:
	pugi::xml_document def_document;
	pugi::xml_node definitions;


public:
	void load(char* filename);
	char* lookup(char errCode[]);
	// Create an unloaded instance
	ErrCodeStr();
};
