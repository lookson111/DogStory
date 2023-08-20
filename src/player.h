#pragma once
#include "sdk.h"
#include <boost/json.hpp>
#include <boost/asio/strand.hpp>
#include <string>
#include <random>
#include "model/model.h"
#include "util/token.h"
#include "util/tagged_uuid.h"
#include "ticker.h"

namespace app {
namespace js = boost::json;
namespace net = boost::asio;
namespace sys = boost::system;
using milliseconds = std::chrono::milliseconds;
using Token = security::Token;

namespace detail {
    struct PlayerTag {};
}  // namespace detail

using PlayerId = util::TaggedUUID<detail::PlayerTag>;

class Player {
public:
    Player() = default;
    Player(PlayerId id, model::GameSession* session, model::Dog* dog) :
        session_(session), dog_(dog), id_(id) {}
    const PlayerId& GetId() const {
        return id_;
    }
    const model::Map::Id& MapId() const {
        return session_->MapId();
    }
    std::string_view GetName() const {
        return dog_->GetName();
    }
    const model::GameSession* GetSession() const {
        return session_;
    }
    const model::Dog* GetDog() const {
        return dog_;
    }
    void Move(std::string_view move_cmd);
    Player(const Player& other) {
        *this = other;
    }
    Player(Player&& other) noexcept {
        id_ = std::move(id_);
        session_ = other.session_;
        dog_ = other.dog_;
    }
    Player& operator=(const Player& other) {
        if (this == &other)
            return *this;
        id_ = other.id_;
        session_ = other.session_;
        dog_ = other.dog_;
        return *this;
    }
private:
    PlayerId id_;
    model::GameSession* session_ = nullptr;
    model::Dog* dog_ = nullptr;
};
} // namespace app

namespace util {
template<>
struct TaggedHasher<app::PlayerId> {
    size_t operator () (const app::PlayerId& value) const {
        return boost::hash<typename app::PlayerId::ValueType>()(*value);
    }
};
}

namespace app {
class PlayerTokens {
public:
    using TokenToPlayerContainer = std::unordered_map<Token, Player*,
        util::TaggedHasher<Token>>;
    Player* FindPlayer(Token token) const;
    Token AddPlayer(Player* player);
    void AddToken(Token token, Player* player);
    void DeleteToken(const PlayerId& player_id);
    const TokenToPlayerContainer& GetTokens() const;
    PlayerTokens() = default;
    PlayerTokens(const PlayerTokens& other) {
        token_to_player = other.token_to_player;
    }
    PlayerTokens& operator=(const PlayerTokens& other) {
        token_to_player = other.token_to_player;
        return *this;
    }
private:
    TokenToPlayerContainer token_to_player;
    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::string GetToken();
    // Чтобы сгенерировать токен, получите из generator1_ и generator2_
    // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
    // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
    // чтобы сделать их подбор ещё более затруднительным
    std::string ToHex(uint64_t n) const;
};

class Players {
private:
    using PlayerIdHasher = util::TaggedHasher<PlayerId>;

public:
    using PlayerContainer = std::shared_ptr<Player>;
    using PlayersContainer = std::unordered_map<PlayerId, PlayerContainer, PlayerIdHasher>;
    Player* Add(PlayerId player_id, model::Dog *dog, model::GameSession *session);
    void Add(PlayerContainer&& player);
    Player* FindPlayer(PlayerId player_id, model::Map::Id map_id) noexcept;
    const PlayerId* FindPlayerId(std::string_view player_name) const noexcept;
    Player* FindPlayer(const PlayerId& player_id) const noexcept;
    const PlayersContainer& GetPlayers() const;
    void DeletePlayer(const PlayerId& player_id) noexcept;
    
    PlayersContainer players_;
    Player* PushPlayer(PlayerContainer&& player);
};

class RetiredPlayer {
public:
    RetiredPlayer(PlayerId id, std::string name, model::Score score, milliseconds play_time)
        : id_(std::move(id))
        , name_(std::move(name))
        , score_(score)
        , play_time_(play_time) {
    }
    const PlayerId& GetId() const {
        return id_;
    }
    const std::string& GetName() const {
        return name_;
    }
    model::Score GetScore() const {
        return score_;
    }
    const milliseconds& GetPlayTime() const {
        return play_time_;
    }

private:
    PlayerId id_;
    std::string name_;
    model::Score score_;
    milliseconds play_time_;
};
using RetiredPlayers = std::vector<RetiredPlayer>;

}

