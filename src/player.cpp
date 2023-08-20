#include "player.h"
#include <boost/format.hpp>
#include "log.h"
#include "request_handler/defs.h"

namespace app {
using namespace std::literals;
using namespace defs;

static auto to_booststr = [](std::string_view str) {
    return boost::string_view(str.data(), str.size());
};

void Player::Move(std::string_view move_cmd) {
    model::Move dog_move;
    if (move_cmd == "L"sv) 
        dog_move = model::Move::LEFT;
    else if (move_cmd == "R"sv)
        dog_move = model::Move::RIGHT;
    else if (move_cmd == "U"sv)
        dog_move = model::Move::UP;
    else if (move_cmd == "D"sv)
        dog_move = model::Move::DOWN;
    else 
        dog_move = model::Move::STAND;
    session_->MoveDog(dog_->GetId(), dog_move);    
}

Player* PlayerTokens::FindPlayer(Token token) const
{
    if (token_to_player.contains(token))
        return token_to_player.at(token);
    return nullptr;
}

Token PlayerTokens::AddPlayer(Player* player)
{
    Token token{GetToken()};
    token_to_player[token] = player;
    return token;
}

void PlayerTokens::AddToken(Token token, Player* player) {
    token_to_player[token] = player;
}

void PlayerTokens::DeleteToken(const PlayerId& player_id) {
    for (const auto& [token, player] : token_to_player) {
        if (player->GetId() == player_id) {
            token_to_player.erase(token);
            return;
        }
    }
}

const PlayerTokens::TokenToPlayerContainer& PlayerTokens::GetTokens() const {
    return token_to_player;
}

std::string PlayerTokens::GetToken() {
     std::string r1 = ToHex(generator1_());
     std::string r2 = ToHex(generator2_());
     return r1 + r2;
}

std::string PlayerTokens::ToHex(uint64_t n) const {
    std::string hex;
    //loop runs til n is greater than 0
    while (n > 0) {
        int r = n % 16;
        //remainder converted to hexadecimal digit
        char c = (r < 10) ? ('0' + r) : ('a' + r - 10);
        //hexadecimal digit added to start of the string
        hex = c + hex;
        n /= 16;
    }
    while (hex.size() < 16)
        hex = '0' + hex;
    return hex;
}

Player* Players::PushPlayer(PlayerContainer&& player) {
    const size_t index = players_.size();
    if (auto [it, inserted] = players_.emplace(player->GetId(), player); !inserted) {
        throw std::invalid_argument("Player with id "s + player->GetId().ToString() + " already exists"s);
    }
    return players_.at(player->GetId()).get();
}

Player* Players::Add(PlayerId player_id, model::Dog* dog, model::GameSession* session) {
    auto player = std::make_shared<Player>(player_id, session, dog);
    return PushPlayer(std::move(player));
}

void Players::Add(PlayerContainer&& player) {
    PushPlayer(std::move(player));
}

Player* Players::FindPlayer(PlayerId player_id, model::Map::Id map_id) noexcept {
    if (auto it = players_.find(player_id); it != players_.end()) {
        auto pl = players_.at(player_id).get();
        if (pl->MapId() == map_id)
            return pl;
    }
    return nullptr;
}
const PlayerId* Players::FindPlayerId(std::string_view player_name) const noexcept {
    for (const auto& [id, player] : players_) {
        if (player->GetName() == player_name) {
            return &player->GetId();
        }
    }
    return nullptr;
}

Player* Players::FindPlayer(const PlayerId& player_id) const noexcept {
    if (auto it = players_.find(player_id); it != players_.end()) {
        auto pl = players_.at(player_id).get();
        return pl;
    }
    return nullptr;
}

const Players::PlayersContainer& Players::GetPlayers() const {
    return players_;
}

void Players::DeletePlayer(const PlayerId& player_id) noexcept {
    if (auto it = players_.find(player_id); it != players_.end()) {
        players_.erase(it);
    }
}

};
