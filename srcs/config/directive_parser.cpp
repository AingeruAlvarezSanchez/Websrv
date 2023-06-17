#include <cerrno>
#include <map>
#include "webserv.h"
#include "../classes/server_info.hpp"

//TODO maybe change location of typedef
typedef void(*function)(std::string const&, ServerInfo::s_location &);
typedef std::map< std::string, function > ruleMap;
typedef std::map< std::string, function >::iterator ruleMapIterator;

static std::string erase_directive_delimiters(std::string const& directive, unsigned char const& type) {
    size_t  directiveStart;
    if (type == '{') {
        directiveStart = directive.find_first_of('{');
    } else {
        directiveStart = directive.find_first_of('[');
    }

    size_t  firstNewLine = directive.find_first_of('\n');
    std::string firstLine = directive.substr(directiveStart + 1, (firstNewLine - directiveStart - 1));
    size_t  lineComment = firstLine.find_first_of('#');

    if (firstLine.find_first_not_of(' ') != std::string::npos
        && firstLine.find_first_not_of(' ') != lineComment) {
        errno = 134;
        throw   ServerInfo::BadSyntax("Error: Webserv: Bad syntax"); //TODO maybe as a detail i can give the exact point of error with join
    }
    std::string result = directive.substr((firstNewLine + 1), (directive.length() - (firstNewLine + 1) - 1));
    return result;
}

#include <iostream> //TODO
static void save_location_conf(ServerInfo::s_location & location, std::string const& line) {
    ruleMap locationRuleMap;

    locationRuleMap["limit_except:"] = &set_limit_except_rule;
    locationRuleMap["return:"] = &set_return_rule;
    locationRuleMap["root:"] = &set_root_rule;
    locationRuleMap["try_files:"] = &set_try_files_rule;
    locationRuleMap["auto_index:"] = &set_auto_index_rule;
    locationRuleMap["index:"] = &set_index_rule;
    locationRuleMap["cgi_pass:"] = &set_cgi_pass_rule;
    locationRuleMap["upload:"] = &set_upload_rule;
    for (ruleMapIterator it = locationRuleMap.begin(); it != locationRuleMap.end(); it++) {
        if (line.find(it->first) != std::string::npos
            && line.find(it->first) == line.find_first_not_of(' ')) {
            it->second(line, location);
            return ;
        }
    }
    errno = 134;
    throw   ServerInfo::BadSyntax("Error: Webserv: Bad syntax"); //TODO maybe as a detail i can give the exact point of error with join
}

static void is_valid_comment_line(std::string const& line, size_t const& ruleSemicolon) {
    std::string commentLine = line.substr(ruleSemicolon + 1, (line.length() - ruleSemicolon - 2));

    if (commentLine.find_first_not_of(' ') != std::string::npos
        && commentLine.find_first_not_of(' ') != commentLine.find_first_of('#')) {
        errno = 134;
        throw   ServerInfo::BadSyntax("Error: Webserv: Bad syntax"); //TODO maybe as a detail i can give the exact point of error with join
    }
}

//TODO if something is after the location ] it must throw an exception, it doesnt atm
static ServerInfo::locationDirective    get_location_conf(std::string & locationDirective) {
    size_t locationContentStart = locationDirective.find_first_of('[');
    size_t locationPathStart = locationDirective.find_first_not_of("location: ");
    std::string locationPath = locationDirective.substr(locationPathStart, (locationContentStart - locationPathStart));
    if (locationPath.empty()) {
        errno = 134;
        throw   ServerInfo::BadSyntax("Error: Webserv: Bad syntax"); //TODO maybe as a detail i can give the exact point of error with join
    }
    size_t locationPathEnd = locationPath.find_first_of(' ');
    locationPath.erase(locationPathEnd, locationPath.length());

    locationDirective = erase_directive_delimiters(locationDirective, '[');
    ServerInfo::s_location location = {};
    std::cout << "-------------------------------------------\n";
    while (!locationDirective.empty()) {
        std::string line = locationDirective.substr(0, locationDirective.find_first_of('\n') + 1);
        is_valid_comment_line(line, line.find_first_of(';'));
        if (line.empty()) {
            break ;
        }
        save_location_conf(location, line);

        locationDirective.erase(0, line.length() + 1);
    }
    return std::make_pair(locationPath, location); //TODO
}

//TODO if something is before the rule name (somelocation:, somelisten:) it must throw an exception, it doesnt atm
static void get_rule_conf(std::string const& line, size_t const& ruleSemicolon) {
    is_valid_comment_line(line, ruleSemicolon);
}

ServerInfo::s_serverData   get_directive_conf(std::string & serverDirective) {
    serverDirective = erase_directive_delimiters(serverDirective, '{');

    if (serverDirective.find("listen:") == std::string::npos
        || serverDirective.find("server_name:") == std::string::npos) {
        errno = 134;
        throw   ServerInfo::BadSyntax("Error: Webserv: Bad syntax"); //TODO maybe as a detail i can give the exact point of error with join
    }

    ServerInfo::s_serverData data = {}; //TODO
    while (!serverDirective.empty() && serverDirective.find_first_not_of(" \n") != std::string::npos) {
        std::string line = serverDirective.substr(0, serverDirective.find_first_of('\n') + 1);
        size_t ruleSemicolon = line.find_first_of(';');

        if (ruleSemicolon == std::string::npos
            && line.find("location:") == std::string::npos) {
            std::cout << "Error on line: " << line;
        } else {
            if (line.find("location:") != std::string::npos) {
                size_t locationStart = serverDirective.find("location:");
                size_t locationEnd = serverDirective.find_first_of(']');
                std::string locationDirective = serverDirective.substr(locationStart,((locationEnd + 1) - locationStart));

                serverDirective.erase(locationStart, locationDirective.length() + 1);
                data.serverLocations.push_back(get_location_conf(locationDirective));
            } else {
                get_rule_conf(line, ruleSemicolon);
            }
        }
        serverDirective.erase(0, line.length());
    }
    return data;
}