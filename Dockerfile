# Создать образ на основе базового слоя gcc (там будет ОС и сам компилятор).
# 11.3 — используемая версия gcc.
FROM gcc:11.3 as build

# Выполнить установку зависимостей внутри контейнера.
RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.* 

# копируем conanfile.txt в контейнер и запускаем conan install
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11

# только после этого копируем остальные иходники
COPY ./src /app/src
COPY ./tests /app/tests
COPY ./data /app/data
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . 

# Второй контейнер в том же докерфайле
FROM ubuntu:22.04 as run

RUN apt update && \
    apt install -y rsyslog 
# Создадим пользователя www
RUN groupadd -r www && useradd -r -g www www 
RUN usermod -a -G syslog www
RUN mkdir -p /app/save
RUN chmod a+w /app/save
USER www

# Скопируем приложение со сборочного контейнера в директорию /app.
# Не забываем также папку data, она пригодится.
COPY --from=build /app/build/game_server /app/
COPY ./data /app/data
COPY ./static /app/static
#EXPOSE 30432
ENV GAME_DB_URL='postgres://postgres:Mys3Cr3t@localhost:30432/game_db'


# Запускаем игровой сервер
#ENTRYPOINT ["./app/game_server", "-c", "/app/data/config.json", "-w", "/app/static/", "-t", "50", "--randomize-spawn-points", "--save-state-period", "1000", "--state-file", "/app/save/state.save"] 
ENTRYPOINT ["./app/game_server", "-c", "/app/data/config.json", "-w", "/app/static/", "-t", "20", "--randomize-spawn-points"] 
#ENTRYPOINT ["./app/game_server", "-c", "/app/data/config.json", "-w", "/app/static/", "--randomize-spawn-points"]
