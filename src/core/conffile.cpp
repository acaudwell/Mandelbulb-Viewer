#include "conffile.h"

//section of config file
Regex ConfFile_section("^\\s*\\[([^\\]]+)\\]\\s*$");

// parse key value pair, seperated by an equals sign, removing white space on key and front of the value
Regex ConfFile_key_value("^\\s*([^=\\s]+)\\s*=\\s*([^\\s].*)?$");

// vec2f, vec3f, or vec4f with liberal allowance for whitespace
Regex ConfFile_vec2_value("^\\s*vec2\\(\\s*(-?[0-9.]+)\\s*,\\s*(-?[0-9.]+)\\s*\\)\\s*$");
Regex ConfFile_vec3_value("^\\s*vec3\\(\\s*(-?[0-9.]+)\\s*,\\s*(-?[0-9.]+)\\s*,\\s*(-?[0-9.]+)\\s*\\)\\s*$");
Regex ConfFile_vec4_value("^\\s*vec4\\(\\s*(-?[0-9.]+)\\s*,\\s*(-?[0-9.]+)\\s*,\\s*(-?[0-9.]+)\\s*,\\s*(-?[0-9.]+)\\s*\\)\\s*$");


//ConfEntry

ConfEntry::ConfEntry(std::string name) {
    this->name = name;
}

ConfEntry::ConfEntry(std::string name, std::string value) {
    this->name  = name;
    this->value = value;
}

ConfEntry::ConfEntry(std::string name, bool value) {
    this->name  = name;
    setBool(value);
}

ConfEntry::ConfEntry(std::string name, int value) {
    this->name  = name;
    setInt(value);
}

ConfEntry::ConfEntry(std::string name, float value) {
    this->name  = name;
    setFloat(value);
}

ConfEntry::ConfEntry(std::string name, vec2f value) {
    this->name  = name;
    setVec2(value);
}

ConfEntry::ConfEntry(std::string name, vec3f value) {
    this->name  = name;
    setVec3(value);
}

ConfEntry::ConfEntry(std::string name, vec4f value) {
    this->name  = name;
    setVec4(value);
}

void ConfEntry::setName(std::string name) {
    this->name = name;
}

void ConfEntry::setString(std::string value) {
    this->value = value;
}

void ConfEntry::setFloat(float value) {
    char floattostr[256];
    sprintf(floattostr, "%.5f", value);

    this->value = std::string(floattostr);
}

void ConfEntry::setInt(int value) {
    char inttostr[256];
    sprintf(inttostr, "%d", value);

    this->value = std::string(inttostr);
}

void ConfEntry::setBool(bool value) {
    char booltostr[256];
    sprintf(booltostr, "%s", value==1 ? "yes" : "no");

    this->value = std::string(booltostr);
}

void ConfEntry::setVec2(vec2f value) {
    char vectostr[256];
    sprintf(vectostr, "vec2(%.5f, %.5f)", value.x, value.y);

    this->value = std::string(vectostr);
}

void ConfEntry::setVec3(vec3f value) {
    char vectostr[256];
    sprintf(vectostr, "vec3(%.5f, %.5f, %.5f)", value.x, value.y, value.z);

    this->value = std::string(vectostr);
}

void ConfEntry::setVec4(vec4f value) {
    char vectostr[256];
    sprintf(vectostr, "vec4(%.5f, %.5f, %.5f, %.5f)", value.x, value.y, value.z, value.w);

    this->value = std::string(vectostr);
}

std::string ConfEntry::getName() {
    return name;
}

std::string ConfEntry::getString() {
    return value;
}

int ConfEntry::getInt() {
    return atoi(value.c_str());
}

float ConfEntry::getFloat() {
    return atof(value.c_str());
}

bool ConfEntry::getBool() {

    if(value == "1" || value == "yes" || value == "YES" || value == "Yes")
        return true;

    return false;
}

vec2f ConfEntry::getVec2() {

    std::vector<std::string> matches;

    if(ConfFile_vec2_value.match(value, &matches)) {
        return vec2f(atof(matches[0].c_str()), atof(matches[1].c_str()));
    }

    debugLog("'%s' did not match vec2 regex\n", value.c_str());

    return vec2f(0.0, 0.0);
}

vec3f ConfEntry::getVec3() {

    std::vector<std::string> matches;

    if(ConfFile_vec3_value.match(value, &matches)) {
        return vec3f(atof(matches[0].c_str()), atof(matches[1].c_str()), atof(matches[2].c_str()));
    }

    debugLog("'%s' did not match vec3 regex\n", value.c_str());

    return vec3f(0.0, 0.0, 0.0);
}


vec4f ConfEntry::getVec4() {

    std::vector<std::string> matches;

    if(ConfFile_vec4_value.match(value, &matches)) {
        return vec4f(atof(matches[0].c_str()), atof(matches[1].c_str()), atof(matches[2].c_str()), atof(matches[3].c_str()) );
    }

    debugLog("'%s' did not match vec4 regex\n", value.c_str());

    return vec4f(0.0, 0.0, 0.0, 0.0);
}

//ConfSection

ConfSection::ConfSection() {
}

ConfSection::ConfSection(std::string name) {
    this->name = name;
}

ConfSection::~ConfSection() {
    clear();
}

std::string ConfSection::getName() {
    return name;
}

ConfEntryList* ConfSection::getEntries(std::string key) {
    std::map<std::string, ConfEntryList*>::iterator entry_finder = entrymap.find(key);

    if(entry_finder == entrymap.end()) return 0;

    return entry_finder->second;
}

ConfEntry* ConfSection::getEntry(std::string key) {
    ConfEntryList* entryList = getEntries(key);

    if(entryList==0 || entryList->size()==0) return 0;

    return entryList->front();
}

void ConfSection::addEntry(ConfEntry* entry) {

    ConfEntryList* entrylist = entrymap[entry->getName()];

    if(entrylist==0) {
        entrymap[entry->getName()] = entrylist = new ConfEntryList;
    }

    entrylist->push_back(entry);
}

//replace first entry with that name
void ConfSection::setEntry(ConfEntry* entry) {
    ConfEntryList* entrylist = entrymap[entry->getName()];

    if(entrylist==0) {
        entrymap[entry->getName()] = entrylist = new ConfEntryList;
    }

    //remove any entries with this name
    while(entrylist->size()>0) {
        ConfEntry* front = entrylist->front();
        entrylist->pop_front();
        delete front;
    }

    //add new entry
    entrylist->push_front(entry);
}

void ConfSection::clear() {

    //delete entries
    for(std::map<std::string, ConfEntryList*>::iterator it = entrymap.begin();
        it!= entrymap.end(); it++) {

        ConfEntryList* entrylist = it->second;

        for(std::list<ConfEntry*>::iterator eit = entrylist->begin();
            eit != entrylist->end(); eit++) {

            ConfEntry* e = *eit;
            delete e;
        }

        delete entrylist;
    }

    entrymap.clear();
}

bool ConfSection::hasValue(std::string key) {
    std::string value = getString(key);

    if(value.size()>0) return true;

    return false;
}

std::string ConfSection::getString(std::string key) {
    ConfEntry* entry = getEntry(key);

    if(entry==0) return std::string("");

    return entry->getString();
}

int ConfSection::getInt(std::string key) {
    ConfEntry* entry = getEntry(key);

    if(entry) return entry->getInt();

    return 0;
}

float ConfSection::getFloat(std::string key) {
    ConfEntry* entry = getEntry(key);

    if(entry) return entry->getFloat();

    return 0.0f;
}

bool ConfSection::getBool(std::string key) {
    ConfEntry* entry = getEntry(key);

    if(entry) return entry->getBool();

    return false;
}

vec3f ConfSection::getVec3(std::string key) {
    ConfEntry* entry = getEntry(key);

    if(entry) return entry->getVec3();

    return vec3f(0.0, 0.0, 0.0);
}

vec4f ConfSection::getVec4(std::string key) {
    ConfEntry* entry = getEntry(key);

    if(entry) return entry->getVec4();

    return vec4f(0.0, 0.0, 0.0, 0.0);
}

void ConfSection::print(std::ostream& out) {

    out << "[" << getName() << "]" << std::endl;

    for(std::map<std::string, ConfEntryList*>::iterator it = entrymap.begin();
        it!= entrymap.end(); it++) {

        ConfEntryList* entrylist = it->second;

        for(std::list<ConfEntry*>::iterator eit = entrylist->begin();
            eit != entrylist->end(); eit++) {

            ConfEntry* e = *eit;

            out << e->getName() << "=" << e->getString() << std::endl;
        }
   }

   out << std::endl;
}

//ConfFile

ConfFile::ConfFile() {

}

ConfFile::~ConfFile() {
    clear();
}

void ConfFile::clear() {

    conf_error = "";

    //delete sections
    for(std::map<std::string, ConfSectionList*>::iterator it = sectionmap.begin();
        it!= sectionmap.end(); it++) {

        ConfSectionList* sectionlist = it->second;

        for(std::list<ConfSection*>::iterator sit = sectionlist->begin();
            sit != sectionlist->end(); sit++) {

            ConfSection* s = *sit;
            delete s;
        }

        delete sectionlist;
    }

    sectionmap.clear();
}

void ConfFile::setFilename(std::string filename) {
    this->conffile = filename;
}

std::string ConfFile::getFilename() {
    return conffile;
}

std::string ConfFile::getError() {
    return conf_error;
}

bool ConfFile::save(std::string conffile) {
    this->conffile = conffile;
    return save();
}

bool ConfFile::save() {
    if(conffile.size()==0) return false;

    //save conf file
    std::ofstream out(conffile.c_str());

    for(std::map<std::string, ConfSectionList*>::iterator it = sectionmap.begin();
        it!= sectionmap.end(); it++) {

        ConfSectionList* sectionlist = it->second;

        for(std::list<ConfSection*>::iterator sit = sectionlist->begin();
            sit != sectionlist->end(); sit++) {

            ConfSection* s = *sit;

            s->print(out);
        }
    }

    return true;
}

bool ConfFile::load(std::string conffile) {
    this->conffile = conffile;
    return load();
}

bool ConfFile::load() {
    debugLog("ConfFile::load(%s)\n", conffile.c_str());

    clear();

    if(conffile.size()==0) return false;

    char buff[1024];

    int lineno = 0;
    ConfSection* sec = 0;

    std::ifstream in(conffile.c_str());

    if(!in.is_open()) {
        sprintf(buff, "failed to open config file %s", conffile.c_str());
        conf_error = std::string(buff);
        return false;
    }

    std::string whitespaces (" \t\f\v\n\r");
    std::string line;

    while(std::getline(in, line)) {

        lineno++;

        std::vector<std::string> matches;

        // blank line or commented out lines
        if(line.size() == 0 || line.size() > 0 && line[0] == '#') {

            continue;

        // sections
        } else if(ConfFile_section.match(line, &matches)) {

            if(sec != 0) addSection(sec);

            sec = new ConfSection(matches[0]);

        // key value pairs
        } else if(ConfFile_key_value.match(line, &matches)) {

            std::string key   = matches[0];
            std::string value = (matches.size()>1) ? matches[1] : "";

            //trim whitespace
            if(value.size()>0) {
                size_t string_end = value.find_last_not_of(whitespaces);

                if(string_end == std::string::npos) value = "";
                else if(string_end != value.size()-1) value = value.substr(0,string_end+1);
            }

            if(sec==0) sec = new ConfSection("");

            sec->addEntry(new ConfEntry(key, value));

            debugLog("%s: [%s] %s => %s\n", conffile.c_str(), sec->getName().c_str(), key.c_str(), value.c_str());

        } else {
            sprintf(buff, "failed to read line %d of config file %s", lineno, conffile.c_str());
            conf_error = std::string(buff);
            return false;
        }
    }

    if(sec != 0) addSection(sec);

    in.close();

    return true;
}

bool ConfFile::hasValue(std::string section, std::string key) {
    std::string value = getString(section, key);

    if(value.size()>0) return true;

    return false;
}

bool ConfFile::hasSection(std::string section) {

    ConfSection* sec = getSection(section);

    if(sec==0) return false;

    return true;
}

void ConfFile::setSection(ConfSection* section) {

    ConfSectionList* sectionlist = getSections(section->getName());

    if(sectionlist==0) {
        sectionmap[section->getName()] = sectionlist = new ConfSectionList;
    }

    if(sectionlist->size() != 0) {
        ConfSection* front = sectionlist->front();
        sectionlist->pop_front();
        delete front;
    }

    sectionlist->push_back(section);
}

void ConfFile::addSection(ConfSection* section) {

    ConfSectionList* sectionlist = getSections(section->getName());

    if(sectionlist==0) {
        sectionmap[section->getName()] = sectionlist = new ConfSectionList;
    }

    sectionlist->push_back(section);
}

//returns the list of all sections with a particular name
ConfSectionList* ConfFile::getSections(std::string section) {
    std::map<std::string, ConfSectionList*>::iterator section_finder = sectionmap.find(section);

    if(section_finder == sectionmap.end()) return 0;

    return section_finder->second;
}

//returns the first section with a particular name
ConfSection* ConfFile::getSection(std::string section) {

    ConfSectionList* sectionlist = getSections(section);

    if(sectionlist==0 || sectionlist->size()==0) return 0;

    return sectionlist->front();
}

//returns a list of all entries in a section with a particular name
ConfEntryList* ConfFile::getEntries(std::string section, std::string key) {

    ConfSection* sec = getSection(section);

    if(sec==0) return 0;

    ConfEntryList* entryList = sec->getEntries(key);

    return entryList;
}

//get first entry in a section with a particular name
ConfEntry* ConfFile::getEntry(std::string section, std::string key) {

    ConfSection* sec = getSection(section);

    if(sec==0) return 0;

    ConfEntry* entry = sec->getEntry(key);

    return entry;
}

std::string ConfFile::getString(std::string section, std::string key) {

    ConfEntry* entry = getEntry(section, key);

    if(entry==0) return std::string("");

    return entry->getString();
}

int ConfFile::getInt(std::string section, std::string key) {
    ConfEntry* entry = getEntry(section, key);

    if(entry) return entry->getInt();

    return 0;
}

float ConfFile::getFloat(std::string section, std::string key) {
    ConfEntry* entry = getEntry(section, key);

    if(entry) return entry->getFloat();

    return 0.0f;
}

bool ConfFile::getBool(std::string section, std::string key) {
    ConfEntry* entry = getEntry(section, key);

    if(entry) return entry->getBool();

    return false;
}

vec3f ConfFile::getVec3(std::string section, std::string key) {
    ConfEntry* entry = getEntry(section, key);

    if(entry) return entry->getVec3();

    return vec3f(0.0, 0.0, 0.0);
}

vec4f ConfFile::getVec4(std::string section, std::string key) {
    ConfEntry* entry = getEntry(section, key);

    if(entry) return entry->getVec4();

    return vec4f(0.0, 0.0, 0.0, 0.0);
}
