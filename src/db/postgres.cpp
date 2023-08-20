#include "postgres.h"

#include <pqxx/zview.hxx>
#include <pqxx/pqxx>
#include <iostream>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void RetiredPlayerRepositoryImpl::Save(const app::RetiredPlayer& retired_player) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    auto conn = conn_pool_.GetConnection();
    pqxx::work work{*conn};
    work.exec_params(
        R"(
INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO NOTHING;
)"_zv,
        retired_player.GetId().ToString(), 
        retired_player.GetName(),
        retired_player.GetScore(),
        retired_player.GetPlayTime().count()
    );
    work.commit();
}

app::RetiredPlayers RetiredPlayerRepositoryImpl::Get(unsigned offset, unsigned limit) {
    auto conn = conn_pool_.GetConnection();
    pqxx::read_transaction r{*conn};
    app::RetiredPlayers retired_players;
    auto query_text = "SELECT id, name, score, play_time_ms FROM retired_players "
        "ORDER BY score DESC, play_time_ms ASC, name ASC "
        "LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset) + ";";
    auto res =  r.query<std::string, std::string, unsigned, unsigned>(query_text);
    for (const auto [id, name, score, play_time] : res) {
        app::RetiredPlayer retired_player(app::PlayerId::FromString(id), name, score, app::milliseconds{play_time});
        retired_players.push_back(std::move(retired_player));
    }
    return retired_players;
}

Database::Database(size_t capacity, const std::string& db_url)
    : conn_pool_{capacity, [db_url] {
        return std::make_shared<pqxx::connection>(db_url);
     }} {
    auto conn = conn_pool_.GetConnection();
    pqxx::work work{*conn};
    std::cout << "Create table" << std::endl;
    work.exec(R"(
CREATE TABLE IF NOT EXISTS retired_players (
    id UUID CONSTRAINT player_id_constraint PRIMARY KEY,
    name varchar(100) NOT NULL,
    score integer NOT NULL,
    play_time_ms integer NOT NULL
);
)"_zv);
    // коммитим изменения
    work.commit();
}

}  // namespace postgres
