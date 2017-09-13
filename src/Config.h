/* * * * * * * * * * * * * * * * *
 * Congestion proxy application  *
 * Author: Martin Ubl            *
 *         kenny@cetes.cz        *
 * * * * * * * * * * * * * * * * */

#pragma once

#include "Singleton.h"

// character used as comment indicator
constexpr char ConfigCommentCharacter = '#';
// character used as delimiter between key and value
constexpr char ConfigKeyValueSplitCharacter = '=';

// default config local path - first priority
constexpr char* ConfigDefaultLocalFilename = "congestion-proxy.conf";
// default system config path - second priority
constexpr char* ConfigDefaultSystemFilename = "/etc/congestion-proxy.conf";

// all config keys present in config
enum class ConfigKeys
{
    PROXY_BIND,
    PROXY_PORT,
    MESSAGE_RATE_INTERVAL,
    MESSAGE_RATE_LIMIT,
    PROXY_COMMAND,
    GROUPING_DELAY,
    CONGESTION_MESSAGE,
    MESSAGE_KEYWORDS,
};

// 1:1 mapping of config keys to config strings
static std::vector<std::string> ConfigKeyNames = {
    "proxy-bind",
    "proxy-port",
    "message-rate-interval",
    "message-rate-limit",
    "proxy-command",
    "grouping-delay",
    "congestion-message",
    "message-keywords"
};

// config option structure - represented as both string and long signed integer
struct ConfigOption
{
    std::string stringVal;
    int64_t intVal;
};

// config singleton class - stores config settings and preparsed values
class Config
{
    public:
        Config();

        // loads config; returns true on success, false on failure
        bool Load();

        // retrieves config integer value
        int64_t GetIntValue(ConfigKeys key) const;
        // retrieves config string value
        std::string GetStringValue(ConfigKeys key) const;

        // retrieves stored keywords
        std::vector<std::string> const& GetKeywords() const;

    protected:
        // parses keywords from config option to internal vector
        void ParseKeywords();

    private:
        // map of config options
        std::map<ConfigKeys, ConfigOption> m_configOptions;
        // vector of all keywords loaded from config file
        std::vector<std::string> m_keywords;
};

#define sConfig Singleton<Config>::getInstance()
