/* * * * * * * * * * * * * * * * *
 * Congestion proxy application  *
 * Author: Martin Ubl            *
 *         kenny@cetes.cz        *
 * * * * * * * * * * * * * * * * */

#include "general.h"
#include "Config.h"

// trim std::string from whitespaces
static inline void trim(std::string &s)
{
    // trim left
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));

    // trim right
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// does specified file exist?
static bool probe_file_existence(const char* path)
{
    // use C-style approach, since it's faster

    FILE* f;
#ifdef _WIN32
    int err = fopen_s(&f, path, "r");

    if (f && err == 0)
    {
        fclose(f);
        return true;
    }
#else
    f = fopen(path, "r");

    if (f)
    {
        fclose(f);
        return true;
    }
#endif

    return false;
}

Config::Config()
{
    //
}

bool Config::Load()
{
    std::string configFileName;

    // determine config location; prefer local config, then look to /etc/ folder

    if (probe_file_existence(ConfigDefaultLocalFilename))
        configFileName = ConfigDefaultLocalFilename;
    else if (probe_file_existence(ConfigDefaultSystemFilename))
        configFileName = ConfigDefaultSystemFilename;
    else
    {
        std::cerr << "Could not find config file congestion-proxy.conf" << std::endl;
        return false;
    }

    // load config
    std::ifstream cf(configFileName);

    if (!cf.is_open())
    {
        std::cerr << "Failed to open config file: " << configFileName << std::endl;
        return false;
    }

    std::map<std::string, std::string> readOptions;

    // read all lines, split by '=', store
    std::string line;
    while (!cf.eof())
    {
        std::getline(cf, line);

        trim(line);

        if (line.empty() || line[0] == ConfigCommentCharacter)
            continue;

        size_t eqpos = line.find(ConfigKeyValueSplitCharacter);

        std::string opt = line.substr(0, eqpos);
        std::string val = line.substr(eqpos + 1);

        trim(opt);
        trim(val);

        readOptions[opt] = val;
    }

    // analyze config options, store as integer and string
    for (auto copt : readOptions)
    {
        size_t i;
        for (i = 0; i < ConfigKeyNames.size(); i++)
        {
            if (ConfigKeyNames[i] == copt.first)
                break;
        }

        if (i == ConfigKeyNames.size())
        {
            std::cerr << "Unrecognized config option: " << copt.first << std::endl;
            continue;
        }

        m_configOptions[(ConfigKeys)i].stringVal = copt.second;
        try
        {
            m_configOptions[(ConfigKeys)i].intVal = std::stoll(copt.second);
        }
        catch (...)
        {
            //
        }
    }

    // parse keywords option
    ParseKeywords();

    return true;
}

int64_t Config::GetIntValue(ConfigKeys key) const
{
    auto itr = m_configOptions.find(key);
    if (itr == m_configOptions.end())
        return 0;

    return itr->second.intVal;
}

std::string Config::GetStringValue(ConfigKeys key) const
{
    auto itr = m_configOptions.find(key);
    if (itr == m_configOptions.end())
        return "";

    return itr->second.stringVal;
}

std::vector<std::string> const& Config::GetKeywords() const
{
    return m_keywords;
}

void Config::ParseKeywords()
{
    std::string val = GetStringValue(ConfigKeys::MESSAGE_KEYWORDS);

    if (val.empty())
        return;

    size_t pos = 0, tmpos;

    // parse keywords sorrounded by quote character, separated by comma
    while (pos != std::string::npos)
    {
        tmpos = val.find('\"', pos + 1);
        if (tmpos != std::string::npos)
        {
            m_keywords.push_back(val.substr(pos + 1, tmpos - pos - 1));

            // this simply returns npos if no ending sequence found
            tmpos = val.find(',', tmpos);
            tmpos = val.find('"', tmpos);
        }

        pos = tmpos;
    }
}
