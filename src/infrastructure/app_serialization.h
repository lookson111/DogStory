#pragma once

#include "../app.h"
#include <boost/serialization/map.hpp>
#include "model_serialization.h"


namespace app {

    template <typename Archive>
    void serialize(Archive& ar, PlayerId& player, [[maybe_unused]] const unsigned version) {
        ar& (*player);
    }
} // namespace app

namespace serialization {

class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(const app::Player* player)
        : player_id_(player->GetId().ToString())
        , game_session_id_(player->GetSession()->MapId())
        , dog_id_(player->GetDog()->GetId())
    {        
    }

    void Restore(app::App& app) const {
        model::GameSession* session = app.GetGameModel().FindGameSession(game_session_id_);
        model::Dog* dog = session->FindDog(dog_id_);
        app.EditPlayers().Add(app::PlayerId::FromString(player_id_), dog, session);
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& player_id_;
        ar& game_session_id_;
        ar& dog_id_;
    }

private:
    std::string player_id_;
    model::Map::Id game_session_id_{""};
    model::Dog::Id dog_id_{0};
};

class PlayersRepr {
public:
    PlayersRepr() = default;

    explicit PlayersRepr(const app::Players& players) {
        for(auto &[player_id, player] : players.GetPlayers()) 
            players_repr_.push_back(std::move(PlayerRepr(player.get())));
    }

    void Restore(app::App& app) const {
        for (auto &player_repr : players_repr_) {
            player_repr.Restore(app);
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& players_repr_;
    }

private:
    std::list<PlayerRepr> players_repr_;
};

class PlayerTokensRepr {
public:
    PlayerTokensRepr() = default;

    explicit PlayerTokensRepr(const app::PlayerTokens& player_tokens)
    {
        for (auto &player_token : player_tokens.GetTokens()) {
            player_tokens_[*player_token.first] = player_token.second->GetId().ToString();
        }
    }

    void Restore(app::App& app) const {
        for (auto &player_token : player_tokens_) {
            app.EditPlayerTokens().AddToken(security::Token{ player_token.first },
                app.EditPlayers().FindPlayer(app::PlayerId::FromString(player_token.second)));
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& player_tokens_;
    }

private:
    std::map<std::string, std::string> player_tokens_;
};

class AppRepr {
public:
    AppRepr() = default;

    explicit AppRepr(app::App& app)
        : game_repr_(app.GetGameModel())
        , players_repr_(app.GetPlayers())
        , player_tokens_repr_(app.GetPlayerTokens()) 
    {
    }

    void Restore(app::App& app) const {
        game_repr_.Restore(app.GetGameModel());
        players_repr_.Restore(app);
        player_tokens_repr_.Restore(app);
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& game_repr_;
        ar& players_repr_;
        ar& player_tokens_repr_;
    }

private:
    GameRepr game_repr_;
    PlayersRepr players_repr_;
    PlayerTokensRepr player_tokens_repr_;
};

}  // namespace serialization
