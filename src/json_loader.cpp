#include "json_loader.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/json.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <iostream>

namespace json_loader {
using namespace boost::property_tree;
using namespace std::literals;
namespace fs = std::filesystem;
namespace js = boost::json;

static constexpr size_t default_bag_capacity = 3;

model::Road LoadRoad(ptree &ptreeRoad, geom::Dimension2D road_offset) {
    model::Point start;
    if (ptreeRoad.to_iterator(ptreeRoad.find("y1")) == ptreeRoad.end()) {
        start.x = ptreeRoad.get<int>("x0");
        start.y = ptreeRoad.get<int>("y0");
        return model::Road(model::Road::HORIZONTAL, start, 
            ptreeRoad.get<int>("x1"), road_offset);
    }
    start.x = ptreeRoad.get<int>("x0");
    start.y = ptreeRoad.get<int>("y0");
    return model::Road(model::Road::VERTICAL, start, 
        ptreeRoad.get<int>("y1"), road_offset);
}

model::Building LoadBuilding(ptree &ptB) {
    model::Rectangle r{.position = {.x = ptB.get<int>("x"), .y = ptB.get<int>("y")},
        .size = {.width = ptB.get<int>("w"), .height = ptB.get<int>("h")}};
    return model::Building(r);
}

model::Office LoadOffice(ptree &ptO) {
    model::Office::Id idO{ptO.get<std::string>("id")};
    return model::Office(idO, 
        model::Point{.x = ptO.get<int>("x"), .y = ptO.get<int>("y")}, 
        model::Offset{.dx = ptO.get<int>("offsetX"), .dy = ptO.get<int>("offsetY")});
}

template<typename T>
auto GetDefaultParam(ptree& ptree, const std::string& param, T default_value) {
    T value;
    if (ptree.to_iterator(ptree.find(param)) != ptree.end()) {
        value = ptree.get<T>(param);
    }
    else {
        value = default_value;
    }
    return value;
}

model::Map LoadMap(ptree &ptreeMap, const model::DefaultMapParam &def_param) {
    auto id   = ptreeMap.get<std::string>("id");
    auto name = ptreeMap.get<std::string>("name");
    model::DefaultMapParam def_map_param;
    def_map_param.dog_speed = GetDefaultParam(ptreeMap, "dogSpeed"s, def_param.dog_speed);
    def_map_param.bag_capacity = GetDefaultParam(ptreeMap, "bagCapacity"s, def_param.bag_capacity);

    model::Map::Id idmap{id};
    model::Map map(idmap, name, def_map_param);

    ptree jroads = ptreeMap.get_child("roads");
    BOOST_FOREACH(ptree::value_type &jroad, jroads) {
        map.AddRoad(LoadRoad(jroad.second, map.GetRoadOffset()));
    }
    ptree jbuildings = ptreeMap.get_child("buildings");
    BOOST_FOREACH(ptree::value_type &jbuilding, jbuildings) {
        map.AddBuilding(LoadBuilding(jbuilding.second));
    }
    ptree joffices = ptreeMap.get_child("offices");
    BOOST_FOREACH(ptree::value_type &joffice, joffices) {
        map.AddOffice(LoadOffice(joffice.second));
    }
    ptree jloots_param = ptreeMap.get_child("lootTypes");
    BOOST_FOREACH(auto& jloot_param, jloots_param) {
        map.AddLoot(jloot_param.second.get<model::LootParam>("value"));
    }
    return map;
}

model::ExtraData LoadExtraData(const fs::path& json_path) {
    model::ExtraData extra_data;
    js::error_code ec;
    std::string json_string;
    std::ifstream json_file(json_path);
    if (!json_file.is_open())
        throw std::logic_error("Json file error open."s);
    std::stringstream buffer;
    buffer << json_file.rdbuf();
    json_string = buffer.str();
    js::value const jv = js::parse(json_string, ec);
    if (ec)
        throw std::logic_error("Json file read error: "s + ec.what());
    auto json_maps = jv.at("maps");
    for (auto& json_map : json_maps.get_array()) {
        auto id = json_map.at("id").as_string();
        auto json_loot_types = json_map.at("lootTypes");
        size_t cnt = json_loot_types.as_array().size();
        if (cnt < 1)
            throw std::logic_error("The map must contains at least one item!");
        extra_data.SetLootTypes(id.c_str(), serialize(json_loot_types), static_cast<int>(cnt));
    }
    return extra_data;
}

model::LootGeneratorConfig LoadLootGenConfig(ptree& ptree) {
    auto loot_conf = ptree.get_child("lootGeneratorConfig");
    model::LootGeneratorConfig lgc;
    lgc.period = std::chrono::milliseconds(
        static_cast<int>(loot_conf.get<double>("period")*sec_to_ms));
    lgc.probability = loot_conf.get<double>("probability");
    return lgc;
}

std::unique_ptr<model::Game> LoadGame(const fs::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    ptree pt;
    try {
        read_json(json_path.generic_string(), pt);
    } 
    catch (ptree_error &e) {
        throw std::logic_error("Json file read error: "s + e.what());
    }
    try {
        model::DefaultMapParam def_param;
        model::GameParam def_game_param;
        def_param.dog_speed = pt.get<double>("defaultDogSpeed");
        def_param.bag_capacity = GetDefaultParam(pt, "defaultBagCapacity", default_bag_capacity);
        auto retire_time_ms = static_cast<int>(
            pt.get<double>("dogRetirementTime") * sec_to_ms);
        def_game_param.dog_retirement_time = std::chrono::milliseconds(retire_time_ms);
        def_game_param.loot_generator_config_ = std::move(LoadLootGenConfig(pt));
        std::unique_ptr<model::Game> game = 
            std::make_unique<model::Game>(def_game_param);
        ptree jmaps = pt.get_child("maps");
        BOOST_FOREACH(ptree::value_type &jmap, jmaps) {
            game->AddMap(LoadMap(jmap.second, def_param));
        }
        game->AddExtraData(LoadExtraData(json_path));
        return std::move(game);
    }
    catch (ptree_error &e) {
        throw std::logic_error("Json parse ptree error: "s + e.what());
    }
}
}  // namespace json_loader
