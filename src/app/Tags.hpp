#ifndef _____TAGLISTING____1
#define _____TAGLISTING____1
#include "../common/PropertyMap.hpp"

void LoadTagsFromBASS (unsigned long handle, PropertyMap& tags, const std::string& file, std::string* displayTitle = nullptr);
bool SaveTags (const std::string& file, const PropertyMap& tags);

#endif
