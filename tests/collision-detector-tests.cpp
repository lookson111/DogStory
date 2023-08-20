#define _USE_MATH_DEFINES
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_contains.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_approx.hpp>
#include <sstream>

#include "../src/model/collision_detector.h"

using namespace collision_detector;
using namespace geom;
using Catch::Matchers::Contains; 
using Catch::Matchers::WithinAbs;
using Catch::Matchers::Predicate;

static constexpr double epsilon = 1e-10;
// Напишите здесь тесты для функции collision_detector::FindGatherEvents
namespace Catch {
    template<>
    struct StringMaker<collision_detector::GatheringEvent> {
        static std::string convert(collision_detector::GatheringEvent const& value) {
            std::ostringstream tmp;
            tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";

            return tmp.str();
        }
    };
    template<>
    struct StringMaker<collision_detector::CollectionResult> {
        static std::string convert(collision_detector::CollectionResult const& value) {
            std::ostringstream tmp;
            tmp << "(sq_distance=" << value.sq_distance << ",proj_ratio=" << value.proj_ratio << ")";
            return tmp.str();
        }
    };
}  // namespace Catch 

bool operator==(const GatheringEvent &lh, const GatheringEvent &rh) {
    return lh.item_id == rh.item_id &&
        lh.gatherer_id == rh.gatherer_id &&
        lh.sq_distance == Catch::Approx(rh.sq_distance).epsilon(epsilon) &&
        lh.time == Catch::Approx(rh.time).epsilon(epsilon);
}

template <typename T>
class IsEqualMatcher : public Catch::Matchers::MatcherBase<T> {
    T m_item_gatherer;
public:
    IsEqualMatcher(T item_gatherer) : m_item_gatherer(item_gatherer) {}

    bool match(T const& in) const override {
        using std::begin;
        using std::end;
        return std::equal(begin(in), end(in), begin(m_item_gatherer), end(m_item_gatherer), 
            [] (const GatheringEvent &a, const GatheringEvent &b) {
                return a == b;
        });
    }

    std::string describe() const override {
        std::ostringstream ss;
        ss << "{";
        for (const auto &v : m_item_gatherer)
            ss << "(" << v.gatherer_id << "," << v.item_id << "," << v.sq_distance << "," << v.time << ")";
        ss << "}";
        return ss.str();
    }
};

template <typename T>
IsEqualMatcher<T> IsEqual(T t) {
    return { t };
}

bool operator==(const Item& lh, const Item &rh) {
        return lh.position == rh.position && lh.width == rh.width;
}
bool operator==(const Gatherer& lh, const Gatherer &rh) {
        return lh.start_pos == rh.start_pos && lh.end_pos == rh.end_pos && lh.width == rh.width;
}

bool operator==(const CollectionResult& lh, const CollectionResult &rh) {
        return true;
}


static const ItemGatherer::Items one_item = { Item{.position{5.0, 2.0}, .width = 1.0 } };
static const ItemGatherer::Items one_item_1 = { Item{.position{5.0, 3}, .width = 1.0 } };
static const ItemGatherer::Items one_item_2 = { Item{.position{5.0, 2}, .width = 0.99 } };
static const ItemGatherer::Items one_item_3 = { Item{.position{11.0, 0}, .width = 2 } };
static const ItemGatherer::Items one_item_4 = { Item{.position{10.0, 2}, .width = 1 } };
static const ItemGatherer::Items one_item_5 = { Item{.position{10.0, 0}, .width = 1 } };
static const ItemGatherer::Items one_item_6 = { Item{.position{5.0, 0}, .width = 0.5 } };
static const ItemGatherer::Items one_item_7 = { Item{.position{5.0, 0}, .width = 2.0 } };

static const ItemGatherer::Items one_item_inclined_track_1 = { Item{.position{3.0, 6.0}, .width = 1.79 } };
static const ItemGatherer::Items one_item_inclined_track_2 = { Item{.position{3.0, 6.0}, .width = 1.78 } };


static const ItemGatherer::Items subsequence_items = { 
    Item{.position{1.97, 2.76}, .width = 0.94 },
    Item{.position{1.36, 2.44}, .width = 0.46 }, 
    Item{.position{6.00, 5.00}, .width = 0.45 }};

static const ItemGatherer::Gatherers one_gatherer =
    { Gatherer{.start_pos{0.0, 0.0}, .end_pos{10.0, 0.0}, .width = 1.0} };
static const ItemGatherer::Gatherers one_gatherer_inclined_track =
    { Gatherer{.start_pos{-0.5, 1.0}, .end_pos{9.5, 6.0}, .width = 1.12} };

auto gen_lamda(const auto &ge_exp) {
    return [&ge_exp](GatheringEvent in) {
        return in == ge_exp;
    };
}
void contains_gather_event(ItemGatherer &item_gatherer, const ItemGatherer::Items &one_item, const GatheringEvent &ge_exp) {
    item_gatherer.Set(one_item);
    auto ge_res = FindGatherEvents(item_gatherer);
    CHECK_THAT(ge_res, Contains(Predicate<GatheringEvent>(gen_lamda(ge_exp)))); 
}
void not_contains_gather_event(ItemGatherer &item_gatherer, const ItemGatherer::Items &one_item, const GatheringEvent &ge_exp) {
    item_gatherer.Set(one_item);
    auto ge_res = FindGatherEvents(item_gatherer);
    CHECK_THAT(ge_res, !Contains(Predicate<GatheringEvent>(gen_lamda(ge_exp)))); 
}

SCENARIO("Collision detector", "[Collision detector]") {
    GIVEN("Item gatherer container") {
        ItemGatherer item_gatherer;
        WHEN("Conteiner emtpy") {
            CHECK(item_gatherer.GatherersCount() == 0);
            CHECK(item_gatherer.ItemsCount() == 0);
        }
        THEN("Add one item") {
            item_gatherer.Set(one_item);
            THEN("one item") {
                CHECK(item_gatherer.ItemsCount() == 1);
            }
            AND_THEN("check item") {
                CHECK(item_gatherer.GetItem(0) == one_item[0]);
            }
        }
        AND_THEN("Add gatherer") {
            item_gatherer.Set(one_gatherer);
            THEN("one getharer") {
                CHECK(item_gatherer.GatherersCount() == 1);
            }
            AND_THEN("check gatherer") {
                CHECK(item_gatherer.GetGatherer(0) == one_gatherer[0]);
            }
        }
    }
    AND_GIVEN("Track and points") {
        WHEN("line horizontal") {
            // линия по абсцисс точка пересекается
            auto coll = TryCollectPoint(Point2D{0, 0}, Point2D{10, 0}, Point2D{5, 2});
            WHEN("Point distributed by line") {
                CHECK(coll.IsCollected(2));
            }
            AND_WHEN("Point not distributed by line") {
                CHECK(!coll.IsCollected(1));
            }
        }
        AND_WHEN("line vertical") {
            // линия по ординате точка пересекается
            auto coll = TryCollectPoint(Point2D{0, 0}, Point2D{0, 10}, Point2D{2, 5});
            WHEN("Point distributed by line") {
                CHECK(coll.IsCollected(2));
            }
            AND_WHEN("Point not distributed by line") {
                CHECK(!coll.IsCollected(1));
            }
        }
        AND_WHEN("Point in line vertical") {
            auto coll = TryCollectPoint(Point2D{0, 0}, Point2D{0, 10}, Point2D{0, 5});
            WHEN("Point") {
                CHECK(coll.IsCollected(1));
            }
            AND_WHEN("Point") {
                CHECK(coll.IsCollected(10));
            }
        }
        AND_WHEN("Point out abscissa line vertical") {
            auto coll = TryCollectPoint(Point2D{0, 0}, Point2D{0, 10}, Point2D{12, 2});
            WHEN("Point does not intersect") {
                CHECK(!coll.IsCollected(3));
                CHECK(!coll.IsCollected(2));
            }
        }
        AND_WHEN("Point out abscissa oblique line") {
            auto coll = TryCollectPoint(Point2D{0, 0}, Point2D{8, -6}, Point2D{6, -2});
            WHEN("Point does intersect") {
                CHECK(coll.IsCollected(2));
            }
            AND_WHEN("Point does not intersect") {
                CHECK(!coll.IsCollected(1.9));
            }
        }
    }
    AND_GIVEN("Item gatherer container for check collision") {
        ItemGatherer item_gatherer;
        THEN("add one item") {
            item_gatherer.Set(one_item);
            WHEN("add one item") {
                CHECK(item_gatherer.ItemsCount() == 1);
            }
        }
        AND_THEN("add one gatheres") {
            item_gatherer.Set(one_gatherer);
            WHEN("add one gatherer") {
                CHECK(item_gatherer.GatherersCount() == 1);
            }
        }
        AND_THEN("One gatherer get one item") {
            item_gatherer.Set(one_gatherer);
            WHEN("capture item") {
                contains_gather_event(item_gatherer, one_item, 
                    GatheringEvent{.item_id = 0, .gatherer_id = 0, .sq_distance = 4.0, .time = 0.5});
            }
            AND_WHEN("not capture item") {
                not_contains_gather_event(item_gatherer, one_item_1, 
                    GatheringEvent{.item_id = 0, .gatherer_id = 0, .sq_distance = 9.0, .time = 0.5});
            }
            AND_WHEN("not capture item") {
                not_contains_gather_event(item_gatherer, one_item_2, 
                    GatheringEvent{.item_id = 0, .gatherer_id = 0, .sq_distance = 4.0, .time = 0.5});
            }
            AND_WHEN("not capture item") {
                not_contains_gather_event(item_gatherer, one_item_3, 
                    GatheringEvent{.item_id = 0, .gatherer_id = 0, .sq_distance = 4.0, .time = 0.5});
            }
            AND_WHEN("capture item") {
                contains_gather_event(item_gatherer, one_item_4, 
                    GatheringEvent{.item_id = 0, .gatherer_id = 0, .sq_distance = 4.0, .time = 1.0});
            }
            AND_WHEN("item inside 1") {
                contains_gather_event(item_gatherer, one_item_5, 
                    GatheringEvent{.item_id = 0, .gatherer_id = 0, .sq_distance = 0.0, .time = 1.0});
            }
            AND_WHEN("item inside 2") {
                contains_gather_event(item_gatherer, one_item_6, 
                    GatheringEvent{.item_id = 0, .gatherer_id = 0, .sq_distance = 0.0, .time = 0.5});
            }
            AND_WHEN("item inside 3") {
                contains_gather_event(item_gatherer, one_item_7, 
                    GatheringEvent{.item_id = 0, .gatherer_id = 0, .sq_distance = 0.0, .time = 0.5});
            }
        }
        AND_THEN("inclined track") {
            item_gatherer.Set(one_gatherer_inclined_track);
            WHEN("capture item") {
                contains_gather_event(item_gatherer, one_item_inclined_track_1, 
                    GatheringEvent{.item_id = 0, .gatherer_id = 0, .sq_distance = 8.45, .time = 0.48});
            }
            AND_WHEN("not capture item")  {
                not_contains_gather_event(item_gatherer, one_item_inclined_track_2, 
                    GatheringEvent{.item_id = 0, .gatherer_id = 0, .sq_distance = 8.45, .time = 0.48});
            }
            AND_WHEN("subsequence items")  {
                item_gatherer.Set(subsequence_items);
                std::vector<GatheringEvent> ge_exp{
                    {.item_id = 1, .gatherer_id = 0, .sq_distance = 0.20808, .time = 0.2064},
                    {.item_id = 0, .gatherer_id = 0, .sq_distance = 0.2205,  .time = 0.268},
                    {.item_id = 2, .gatherer_id = 0, .sq_distance = 0.45,    .time = 0.68}};
                auto ge_res = FindGatherEvents(item_gatherer);
                CHECK_THAT(ge_res, IsEqual(ge_exp)); 
            }
        }
    }
}   

