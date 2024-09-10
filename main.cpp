#include "tgbot/tgbot.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <map>
#include <sstream>

// Отримання API-ключа
std::string getApiKey()
{
    return "4e41bb27200ce51d8b334a0591f28a68"; // Ваш API-ключ OpenWeatherMap
}

// Функція для отримання емодзі в залежності від опису погоди
std::string getWeatherEmoji(const std::string& description)
{
    std::map<std::string, std::string> emojiMap = {
        {"clear sky", "☀️"},
        {"few clouds", "🌤️"},
        {"scattered clouds", "☁️"},
        {"broken clouds", "☁️"},
        {"overcast clouds", "☁️"},
        {"shower rain", "🌧️"},
        {"rain", "🌧️"},
        {"thunderstorm", "⛈️"},
        {"snow", "❄️"},
        {"mist", "🌫️"}
    };

    auto it = emojiMap.find(description);
    return it != emojiMap.end() ? it->second : "🌍";

}

// Функція для отримання прогнозу погоди за координатами
std::string getWeatherForecast(double latitude, double longitude, const std::string& api_key)
{
    std::string url = "https://api.openweathermap.org/data/2.5/weather?lat="
        + std::to_string(latitude)
        + "&lon=" + std::to_string(longitude)
        + "&appid=" + api_key
        + "&units=metric";

    cpr::Response r = cpr::Get(cpr::Url{url});

    if (r.status_code == 200)
    {
        auto json = nlohmann::json::parse(r.text);

        std::string description = json["weather"][0]["description"];
        double temp = json["main"]["temp"];
        double feels_like = json["main"]["feels_like"];
        std::string emoji = getWeatherEmoji(description);

        std::ostringstream oss;
        oss << "Current weather: " << emoji << " " << description << "\n";
        oss << "Temperature: " << temp << "°C (Feels like: " << feels_like << "°C)";

        return oss.str();
    }
    else
    {
        return "Error retrieving weather data: " + std::to_string(r.status_code);
    }
}

int main()
{
    try
    {
        const std::string OPENWEATHERMAP_API_KEY = getApiKey();
        TgBot::Bot bot("7413798060:AAFV-kV1VifzAO4i4kECNlOZ5GqeC9hRigQ");

        SQLite::Database db("bot_database.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        db.exec("CREATE TABLE IF NOT EXISTS locations (user_id INTEGER PRIMARY KEY, latitude REAL, longitude REAL);");

        // Команда /start для показу кнопок
        bot.getEvents().onCommand("start", [&bot](const TgBot::Message::Ptr& message)
        {
            TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

            TgBot::InlineKeyboardButton::Ptr chooseOptionButton(new TgBot::InlineKeyboardButton);
            chooseOptionButton->text = "Choose an option";
            chooseOptionButton->callbackData = "choose_option";

            std::vector<TgBot::InlineKeyboardButton::Ptr> row1 = {chooseOptionButton};

            keyboard->inlineKeyboard.push_back(row1);

            bot.getApi().sendMessage(message->chat->id, "Choose an option:", false, 0, keyboard);
        });

        // Обробка вибору кнопок
        bot.getEvents().onCallbackQuery([&bot, &db, &OPENWEATHERMAP_API_KEY](const TgBot::CallbackQuery::Ptr& query)
        {
            if (query->data == "choose_option")
            {
                TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

                TgBot::InlineKeyboardButton::Ptr myWeatherButton(new TgBot::InlineKeyboardButton);
                myWeatherButton->text = "My Weather";
                myWeatherButton->callbackData = "my_weather";

                TgBot::InlineKeyboardButton::Ptr chooseLocationButton(new TgBot::InlineKeyboardButton);
                chooseLocationButton->text = "Choose Location";
                chooseLocationButton->callbackData = "choose_location";

                std::vector<TgBot::InlineKeyboardButton::Ptr> row1 = {myWeatherButton};
                std::vector<TgBot::InlineKeyboardButton::Ptr> row2 = {chooseLocationButton};

                keyboard->inlineKeyboard.push_back(row1);
                keyboard->inlineKeyboard.push_back(row2);

                bot.getApi().editMessageText("Choose an option:", query->message->chat->id, query->message->messageId,
                                             "", "", false, keyboard);
            }
            else if (query->data == "my_weather")
            {
                // Перевіряємо, чи чат є приватним
                if (query->data == "my_weather")
                {
                    // Перевіряємо, чи чат є приватним
                    if (query->message->chat->type == TgBot::Chat::Type::Private)
                    {
                        TgBot::ReplyKeyboardMarkup::Ptr keyboard(new TgBot::ReplyKeyboardMarkup);
                        keyboard->oneTimeKeyboard = true;
                        keyboard->resizeKeyboard = true;

                        TgBot::KeyboardButton::Ptr locationButton(new TgBot::KeyboardButton);
                        locationButton->text = "Share your location";
                        locationButton->requestLocation = true;

                        std::vector<TgBot::KeyboardButton::Ptr> row;
                        row.push_back(locationButton);
                        keyboard->keyboard.push_back(row);

                        bot.getApi().sendMessage(query->message->chat->id,
                                                 "Please share your location to get the weather forecast.", false, 0,
                                                 keyboard);
                    }
                    else
                    {
                        // У групах і каналах не показуємо кнопку для запиту локації
                        bot.getApi().sendMessage(query->message->chat->id,
                                                 "Location sharing is only available in private chats.");
                    }
                }
            }
        });

        // Обробка місцезнаходження користувача
        bot.getEvents().onAnyMessage([&bot, &db, &OPENWEATHERMAP_API_KEY](const TgBot::Message::Ptr& message)
        {
            if (message->location != nullptr)
            {
                double user_latitude = message->location->latitude;
                double user_longitude = message->location->longitude;

                try
                {
                    SQLite::Statement insertQuery(
                        db, "INSERT OR REPLACE INTO locations (user_id, latitude, longitude) VALUES (?, ?, ?)");
                    insertQuery.bind(1, static_cast<int>(message->from->id));
                    insertQuery.bind(2, user_latitude);
                    insertQuery.bind(3, user_longitude);
                    insertQuery.exec();

                    std::string forecast = getWeatherForecast(user_latitude, user_longitude, OPENWEATHERMAP_API_KEY);
                    bot.getApi().sendMessage(message->chat->id, forecast);
                }
                catch (const std::exception& e)
                {
                    bot.getApi().sendMessage(message->chat->id, "Failed to save your location.");
                }
            }
            else if (!message->text.empty() && message->text[0] != '/')
            {
                bot.getApi().sendMessage(message->chat->id, "Please use /start to share your location.");
            }
        });

        TgBot::TgLongPoll longPoll(bot);
        while (true)
        {
            longPoll.start();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}