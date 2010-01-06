#ifndef CONF_FILE_H
#define CONF_FILE_H

#include <fstream>
#include <string>
#include <map>

#include "regex.h"

typedef std::map<std::string, std::string> string_to_string_map;

class ConfFile {

    std::string conf_error;
    std::string conffile;

    std::map<std::string, string_to_string_map*> confmap;
public:
    ConfFile();
    ~ConfFile();
    void clear();

    bool load(std::string conffile);
    bool load();

    bool hasSection(std::string section);

    bool        hasValue(std::string section, std::string key);
    std::string getString(std::string section, std::string key);
    int         getInt(std::string section, std::string key);
    float       getFloat(std::string section, std::string key);
    bool        getBool(std::string section, std::string key);
    vec3f       getVec3(std::string, std::string key);
    vec4f       getVec4(std::string, std::string key);


    std::string getError();
};

#endif
