#include <stdio.h>
#include <random>
#include <tgbot/tgbot.h>
#define SQLITECPP_COMPILE_DLL
#include <SQLiteCpp/SQLiteCpp.h>

int main() {
    SQLite::Database database("local.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    TgBot::Bot bot("place your bots id");

    bot.getEvents().onCommand("start", [&bot, &database](TgBot::Message::Ptr message) {
        try {
            SQLite::Statement query(database, "SELECT COUNT(*) FROM users WHERE telegram_id = :id");
            query.bind(":id", message->chat->id);

            if (query.executeStep()) {
                auto tmp = query.getColumn(0).getInt();
                if (tmp > 0) {
                    bot.getApi().sendMessage(message->chat->id, u8"Ты уже в игре!");

                    std::cout << "User " << message->chat->id << " is founded" << std::endl;
                }
                else {
                    bot.getApi().sendMessage(message->chat->id, u8"Привет! Я загадаю число от 1 до 100, и тебе надо будет его отгадать!");

                    std::random_device rd;
                    std::mt19937 engine(rd());
                    std::uniform_int_distribution<int> dist(1, 100);
                    int aim = dist(engine);

                    SQLite::Statement query(database, "INSERT INTO users (telegram_id, score, now) VALUES (?, ?, ?)");
                    query.bind(1, message->chat->id);
                    query.bind(2, 0);
                    query.bind(3, aim);
                    query.exec();

                    std::cout << "User " << message->chat->id << " is registred" << std::endl;
                }
            }
        }
        catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    });

    bot.getEvents().onCommand("score", [&bot, &database](TgBot::Message::Ptr message) {
        try {
            std::cout << "User " << message->chat->id << " try get score" << std::endl;

            SQLite::Statement query(database, "SELECT COUNT(*) FROM users WHERE telegram_id = :id");
            query.bind(":id", message->chat->id);

            if (query.executeStep()) {
                auto tmp = query.getColumn(0).getInt();
                if (tmp > 0) {
                    SQLite::Statement query_s(database, "SELECT * FROM users WHERE telegram_id = :id");
                    query_s.bind(":id", message->chat->id);

                    if (query_s.executeStep()) {
                        int result = query_s.getColumn("score");
                        bot.getApi().sendMessage(message->chat->id, u8"Твой счет - " + std::to_string(result));
                    }
                }
                else {
                    bot.getApi().sendMessage(message->chat->id, u8"Для начала нужно начать игру");
                }
            }
        }
        catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    });

    bot.getEvents().onCommand("restart", [&bot, &database](TgBot::Message::Ptr message) {
        try {
            std::cout << "User " << message->chat->id << " try restart" << std::endl;

            SQLite::Statement query(database, "SELECT COUNT(*) FROM users WHERE telegram_id = :id");
            query.bind(":id", message->chat->id);

            if (query.executeStep()) {
                auto tmp = query.getColumn(0).getInt();
                if (tmp > 0) {
                    std::random_device rd;
                    std::mt19937 engine(rd());
                    std::uniform_int_distribution<int> dist(1, 100);
                    int aim = dist(engine);

                    SQLite::Statement query(database, "UPDATE users SET now = :tmp WHERE telegram_id = :id");
                    query.bind(":tmp", aim);
                    query.bind(":id", message->chat->id);
                    query.exec();

                    bot.getApi().sendMessage(message->chat->id, u8"Я загадал новое число");
                }
                else {
                    bot.getApi().sendMessage(message->chat->id, u8"Для начала нужно начать игру");
                }
            }
        }
        catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
        });

    bot.getEvents().onAnyMessage([&database, &bot](TgBot::Message::Ptr message) {
        try {
            if (StringTools::startsWith(message->text, "/start") || StringTools::startsWith(message->text, "/score") || StringTools::startsWith(message->text, "/restart"))
                return;
            std::cout << "User " << message->chat->id << " insert " << message->text << std::endl;
            SQLite::Statement query(database, "SELECT COUNT(*) FROM users WHERE telegram_id = :id");
            query.bind(":id", message->chat->id);

            if (query.executeStep()) {
                auto tmp = query.getColumn(0).getInt();
                if (tmp > 0) {
                    SQLite::Statement query(database, "SELECT * FROM users WHERE telegram_id = :id");
                    query.bind(":id", message->chat->id);

                    int aim = 0;
                    if (query.executeStep()) {
                        aim = query.getColumn("now");

                        int attempt = std::stod(message->text.c_str());

                        if (aim < attempt)
                            bot.getApi().sendMessage(message->chat->id, u8"Много");

                        else if (aim > attempt)
                            bot.getApi().sendMessage(message->chat->id, u8"Мало");

                        else {
                            bot.getApi().sendMessage(message->chat->id, u8"Поздравляю! Число угадано!");
                            std::cout << "User " << message->chat->id << " guessed" << std::endl;

                            std::random_device rd;
                            std::mt19937 engine(rd());
                            std::uniform_int_distribution<int> dist(1, 100);
                            int nw = dist(engine);

                            SQLite::Statement query(database, "UPDATE users SET now = :tmp WHERE telegram_id = :id");
                            query.bind(":tmp", nw);
                            query.bind(":id", message->chat->id);
                            query.exec();

                            SQLite::Statement query_s(database, "SELECT * FROM users WHERE telegram_id = :id");
                            query_s.bind(":id", message->chat->id);

                            if (query_s.executeStep()) {
                                int rs = query_s.getColumn("score");
                                rs++;

                                SQLite::Statement query_u(database, "UPDATE users SET score = :res WHERE telegram_id = :id");

                                query_u.bind(":res", rs);
                                query_u.bind(":id", message->chat->id);
                                query_u.exec();

                                bot.getApi().sendMessage(message->chat->id, u8"Перейдем к новому числу");
                            }
                        }
                    } 
                }
                else {
                    bot.getApi().sendMessage(message->chat->id, u8"Для начала нужно начать игру");
                }
            }
        }
        catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
        });

    try {
        std::cout << "Bot username: " << bot.getApi().getMe()->username.c_str() << std::endl;

        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            std::cout << "LongPoll started" << std::endl;
            longPoll.start();
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}