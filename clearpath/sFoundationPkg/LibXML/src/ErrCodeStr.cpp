#include "pugixml.hpp"
#include "ErrCodeStr.hpp"


void ErrCodeStr::load(char* filename) {
	def_document.load_file(filename);
	definitions = def_document.child("definitions");
}

char* ErrCodeStr::lookup(char errCode[]) {
	pugi::xml_node definition = definitions.find_child_by_attribute("key", errCode);
	return (char*)definition.child_value();
}