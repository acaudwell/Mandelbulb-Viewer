#ifndef CONF_FILE_H
#define CONF_FILE_H

#include <fstream>
#include <string>
#include <list>
#include <map>

#include "regex.h"

class ConfEntry {
    std::string name;
    std::string value;
public:
    ConfEntry();
    ConfEntry(std::string name);
    ConfEntry(std::string name, std::string value);
    ConfEntry(std::string name, int value);
    ConfEntry(std::string name, float value);
    ConfEntry(std::string name, bool value);
    ConfEntry(std::string name, vec2f value);
    ConfEntry(std::string name, vec3f value);
    ConfEntry(std::string name, vec4f value);

    void setName(std::string name);

    void setString(std::string value);
    void setFloat(float value);
    void setInt(int value);
    void setBool(bool value);
    void setVec2(vec2f value);
    void setVec3(vec3f value);
    void setVec4(vec4f value);

    std::string getName();

    std::string getString();
    int         getInt();
    float       getFloat();
    bool        getBool();
    vec2f       getVec2();
    vec3f       getVec3();
    vec4f       getVec4();
};

typedef std::list<ConfEntry*> ConfEntryList;

class ConfSection {
    std::map<std::string, ConfEntryList*> entrymap;
    std::string name;
public:
    ConfSection();
    ConfSection(std::string name);
    ~ConfSection();

    void clear();

    ConfEntry* getEntry(std::string key);
    ConfEntryList* getEntries(std::string key);

    void setEntry(ConfEntry* entry);
    void addEntry(ConfEntry* entry);

    std::string getName();

    bool        hasValue(std::string key);
    std::string getString(std::string key);
    int         getInt(std::string key);
    float       getFloat(std::string key);
    bool        getBool(std::string key);
    vec3f       getVec3(std::string key);
    vec4f       getVec4(std::string key);

    void print(std::ostream& out);
};

typedef std::list<ConfSection*> ConfSectionList;

class ConfFile {

    std::string conf_error;
    std::string conffile;

    std::map<std::string, ConfSectionList*> sectionmap;
public:
    ConfFile();
    ~ConfFile();
    void clear();

    bool load(std::string conffile);
    bool load();

    bool save(std::string conffile);
    bool save();


    void setFilename(std::string filename);
    std::string getFilename();

    bool hasSection(std::string section);
    ConfSection* getSection(std::string section);
    ConfSectionList* getSections(std::string section);

    ConfEntry*   getEntry(std::string section, std::string key);
    ConfEntryList* getEntries(std::string section, std::string key);

    void setSection(ConfSection* section);
    void addSection(ConfSection* section);

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
