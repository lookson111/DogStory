#pragma once

#include <map>
#include <string>
#include <stdexcept>
#include <iostream>
namespace model {
class ExtraData {
public:
	void SetLootTypes(std::string mapID, const std::string& loot_types, int cnt_loot_types);
	std::string_view GetLootTypes(const std::string& mapID) const;
	int GetCntLootTypes(const std::string& mapID) const;
private:
	std::map<std::string, std::pair<std::string, int>> loot_types_in_map_;
};
}
