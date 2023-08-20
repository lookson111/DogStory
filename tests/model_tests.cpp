#include <cmath>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include "../src/model/model.h"
#include "../src/json_loader.h"

using namespace std::literals;

static constexpr double epsilon = 1e-10;

namespace Catch {
	template<>
	struct StringMaker<geom::Point2D> {
		static std::string convert(geom::Point2D const& value) {
			std::ostringstream tmp;
			tmp << "(" << value.x << ", " << value.y << ")";

			return tmp.str();
		}
	};
	template<>
	struct StringMaker<geom::Speed2D> {
		static std::string convert(geom::Speed2D const& value) {
			std::ostringstream tmp;
			tmp << "(" << value.x << ", " << value.y << ")";
			return tmp.str();
		}
	};
}  // namespace Catch 


template<typename T>
bool Equal2D(const T& lh, const T& rh) {
	return
		lh.x == Catch::Approx(rh.x).epsilon(epsilon) &&
		lh.y == Catch::Approx(rh.y).epsilon(epsilon);
}

template <typename T>
class Is2dMatcher : public Catch::Matchers::MatcherBase<T> {
	T m_val;
public:
	Is2dMatcher(T val) : m_val(val) {}

	bool match(T const& in) const override {
		return Equal2D(in, m_val);
	}

	std::string describe() const override {
		std::ostringstream ss;
		ss << "(" << m_val.x << ", " << m_val.y << ")";
		return ss.str();
	}
};

template <typename T>
Is2dMatcher<T> Is2D(T val) {
	return { val };
}

SCENARIO("Game model") {
	using model::Game;
	GIVEN("empty game model") {
		model::GameParam game_param;
		Game game(game_param);
		WHEN("maps not added") {
			CHECK(game.GetMaps().empty());
		}
	}
	AND_GIVEN("a game model") {
		auto game = json_loader::LoadGame("../tests/config_test.json"sv);
		THEN("check loaded tested game model") {
			WHEN("model contains one map") {
				CHECK(game->GetMaps().size() == 1);
			}
		}
		AND_THEN("add game session") {
			auto map_id = game->GetMaps().front().GetId();
			THEN("empty game session") {
				WHEN("model not contains game session") {
					CHECK(game->FindGameSession(map_id) == nullptr);
				}
			}
			AND_THEN("try add game session with bad map id") {
				model::Map::Id bad_id = model::Map::Id{"bad_id"};
				WHEN("add game seesion with bad id") {
					CHECK_THROWS(game->AddGameSession(bad_id));
				}
			}
			AND_THEN("add game session") {
				auto game_session = game->AddGameSession(map_id);
				WHEN("game session is added, and empty") {
					CHECK(game->FindGameSession(map_id) == game_session);
					CHECK(game_session->GetLoots().empty());
				}
				AND_WHEN("dog add") {
					game_session->AddDog("nop");
					THEN("add dog") {
						CHECK(game_session->GetDogs().size() == 1);
					}
				}
			}
			AND_THEN("move dog") {
				game->SetRandomizeSpawnPoints(false);
				auto game_session = game->AddGameSession(map_id);
				auto dog = game_session->AddDog("nop");
				THEN("default direction") {
					CHECK(dog->GetDirection() == "U");
					CHECK_THAT(dog->GetPoint(), Is2D<geom::Point2D>({ 0.0, 0.0 }));
					CHECK_THAT(dog->GetSpeed(), Is2D<geom::Speed2D>({ 0.0, 0.0 }));
				}
				AND_THEN("to west") {
					game_session->MoveDog(dog->GetId(), model::Move::LEFT);
					CHECK(dog->GetDirection() == "L");
					CHECK_THAT(dog->GetSpeed(), Is2D<geom::Speed2D>({-4.0, 0.0 }));
				}
			}
			AND_THEN("move dog tick") {
				game->SetRandomizeSpawnPoints(false);
				auto game_session = game->AddGameSession(map_id);
				auto dog = game_session->AddDog("nop");
				game_session->MoveDog(dog->GetId(), model::Move::RIGHT);
				THEN("move") {
					game->Tick(1000ms);
					CHECK_THAT(dog->GetPoint(), Is2D<geom::Point2D>({ 4.0, 0.0 }));
					CHECK_THAT(dog->GetSpeed(), Is2D<geom::Speed2D>({ 4.0, 0.0 }));
				}
				AND_THEN("movement to the walls") {
					game->Tick(1000s);
					CHECK_THAT(dog->GetPoint(), Is2D<geom::Point2D>({40.4, 0.0 }));
					CHECK_THAT(dog->GetSpeed(), Is2D<geom::Speed2D>({ 0.0, 0.0 }));
				}
				AND_THEN("circling") {
					game->Tick(1000s);
					game_session->MoveDog(dog->GetId(), model::Move::DOWN);
					game->Tick(1000s);
					game_session->MoveDog(dog->GetId(), model::Move::LEFT);
					game->Tick(1000s);
					game_session->MoveDog(dog->GetId(), model::Move::UP);
					game->Tick(1000s);
					CHECK_THAT(dog->GetPoint(), Is2D<geom::Point2D>({-0.4,-0.4 }));
					CHECK_THAT(dog->GetSpeed(), Is2D<geom::Speed2D>({ 0.0, 0.0 }));
				}

			}

		}
	}
}
