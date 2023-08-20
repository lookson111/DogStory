#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "player_repository.h"
#include "pool_connection.h"

namespace postgres {

class RetiredPlayerRepositoryImpl : public app::RetiredPlayerRepository {
public:
    explicit RetiredPlayerRepositoryImpl(ConnectionPool& conn_pool)
        : conn_pool_{conn_pool} {
    }

    void Save(const app::RetiredPlayer& retired_player) override;
    app::RetiredPlayers Get(unsigned offset, unsigned limit) override;

private:
    ConnectionPool& conn_pool_;
};


class Database {
public:
    explicit Database(size_t capacity, const std::string& db_url);

    RetiredPlayerRepositoryImpl& GetRetiredPlayers() & {
        return retired_players_;
    }

private:
    ConnectionPool conn_pool_;
    RetiredPlayerRepositoryImpl retired_players_{ conn_pool_ };
};

}  // namespace postgres
