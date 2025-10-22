#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <fstream>

struct Entry{
    int id;
    int level;
    std::string name;
    std::string type;
    std::string directParent;
    std::vector<std::string> inlineValues;
    std::string value;
};

std::string replaceSingleQuotes(const std::string& input) {
    std::string output = input;
    for (size_t i = 0; i < output.size(); ++i) {
        if (output[i] == '\'') {
            output[i] = '"';
        }
    }
    return output;
}

std::string normalizeAttr(std::string input)
{
    input = replaceSingleQuotes(input);

    //  SystemTime="2025-10-20T13:35:38.8040065Z"/  ->  "SystemTime":"2025-10-20T13:35:38.8040065Z"
    int delimiter = input.find('=');
    std::string modified = "\"";

    if (delimiter == std::string::npos) {
        return input;
    }

    modified += input.replace(delimiter, 1, "\":");  // replace '=' with "':"

    if (!modified.empty() && modified.back() == '/') {
        modified = modified.substr(0, modified.size() - 1);
    }

    return modified;
}

std::vector<std::string> fetchInlineValues(std::string currentKey) 
{
    std::vector<std::string> values;
    std::vector<std::string> returnV;
    std::string tmp;

    for (const char chr : currentKey) {
        if (chr == ' ') {
            values.push_back(tmp);
            tmp.clear();
        } else {
            tmp += chr;
        }
    }

    if (!tmp.empty()) {
        values.push_back(tmp);
    }

    // std::cout << values.size() << std::endl;

    for (int i = 2; i < values.size(); i++) {
        returnV.push_back(normalizeAttr("_" + values[i]));
    }

    return returnV;
}

std::string lastValue(std::vector<std::string> collection, int currentCounter) 
{
    std::string lastValue;
    for (int l = currentCounter - 1; l > 0; l--) {
        if (collection[l][0] == 'o') {
            lastValue = collection[l]; 
            break;
        };
    }

    // if no parent element is found, root is set as parent
    if (lastValue.empty()) lastValue = collection[0];

    return lastValue;
}

std::vector<Entry> findKeys(std::string xmlStr)
{
    std::vector<Entry> finding;
    std::vector<std::string> collection;

    std::string currentKey;
    std::string value;
    std::string lastOpened;
    
    int level = 1;
    bool start = false;
    
    for (int i = 0; i < xmlStr.size(); i++) {
        char chr = xmlStr[i];
        
        if (chr == '<') {
            if (!value.empty()) {
                collection.push_back("v: " + value);
            }
            value.clear();
            start = true;
            continue;

        // closing bracket, so key ends
        } else if (chr == '>') {
            if (!currentKey.empty()) {
                // Self-closing tag: <Tag/>
                if (currentKey.back() == '/') {
                    collection.push_back("s: " + currentKey);
                    lastOpened.clear();
                }
                // Closing tag: </Tag>
                else if (currentKey[0] == '/') {
                    collection.push_back("c: " + currentKey);
                    lastOpened.clear();
                }
                // Opening tag: <Tag>
                else {
                    lastOpened = currentKey;
                    collection.push_back("o: " + currentKey);
                }
            }
            
            currentKey.clear();
            start = false;
            continue;
        }

        if (start == true) {
            currentKey += chr;
        }

        if (start == false) {
            value += chr;
        }
    }

    for (int i = 0; i < collection.size(); i++) {
        std::string element = collection[i];
        // std::cout << element << std::endl;
        char identifier = element[0];
        
        // o = opening; c = closing; s = selfclosing; v = value
        if (identifier == 's') {
            std::vector<std::string> inlineValues = fetchInlineValues(element);
            std::string parent = lastValue(collection, i).substr(3);
            std::string cleanedChild;

            if (element[element.size() - 1] == '/') {
                cleanedChild = element.substr(3, element.size() - 1);
            } else {
                cleanedChild = element.substr(3);
            }

            if (cleanedChild.find(' ') != std::string::npos) {
                cleanedChild = cleanedChild.substr(0, cleanedChild.find(' '));
            }

            finding.push_back({i, level, cleanedChild, "sc", parent, inlineValues, ""});
        } else if (identifier == 'o') {
            std::string value = "";
            std::string parent = lastValue(collection, i).substr(3);
            std::string cleanedChild = element.substr(3);

            if (collection[i + 1][0] == 'v') {
                value = collection[i + 1].substr(3);
            } else {
                value = "{";
            }

            if (cleanedChild.find(' ') != std::string::npos) { 
                cleanedChild = cleanedChild.substr(0, cleanedChild.find(' '));
            }

            finding.push_back({i, level, cleanedChild, "op", parent, fetchInlineValues(element), value});

            level++;
        } else if (identifier == 'c') {
            std::string parent = lastValue(collection, i).substr(3);

            finding.push_back({i, level, element, "cl", parent, fetchInlineValues(element), ""});

            level--;
        }
        
    }

    // for (const auto line : finding) {
    //     std::cout << "ID: " << line.id << 
    //     std::endl << "Level: " << line.level << 
    //     std::endl << "DirectParent: " << line.directParent << 
    //     std::endl << "Name: " << line.name << 
    //     std::endl << "Value: " << line.value << 
    //     std::endl << "Type: " << line.type << std::endl;
    //     if (line.inlineValues.size() > 0) {
    //         std::cout << "inline values: " << std::endl;
    //         for (const auto val : line.inlineValues) {
    //             std::cout << val << std::endl;
    //         } 
    //     }
    //     std::cout << std::endl;
    // }

    return finding;
}

std::string escapeOutput(std::string input)
{
    std::regex trailingCommaPattern(",(\\s*})");
    std::regex newLine("\n");
    std::regex backslash(R"(\\)");

    input = std::regex_replace(input, trailingCommaPattern, "$1");
    input = std::regex_replace(input, newLine, "$1");
    input = std::regex_replace(input, backslash, "\\\\");

    return input;
}

std::string buildString (std::vector<Entry> objects) 
{
    std::string jsonString;
    int highest = 0;

    for (int i = 0; i < objects.size(); i++) {
        Entry line = objects[i];

        if (line.id == 0) {
            jsonString += "{\"" + line.name + "\": " + line.value + line.inlineValues[0] + ", ";
            continue;
        }

        int previous = objects[i - 1].level;

        if (line.type == "sc") {
            if (line.name[line.name.size() - 1] == '/') {
                jsonString += "\"" + line.name + "\": {},\n";
                continue;
            } else {
                jsonString += "\"" + line.name + "\": {";
                for (const auto attr : line.inlineValues) {
                    jsonString += attr + ",";
                }
                // to remove trailing comma ,
                jsonString = jsonString.substr(0, jsonString.length() - 1) + "},\n";
            }
        } else if (line.type == "op") {
            std::string valuePart;
            if (line.inlineValues.size() > 0 && line.value != "" && line.value != "{") {
                // if inlinearg and value
                // inline arg has the name provided and the value has the name _value
                valuePart = "\": \"" + line.value + "\",\n";

                jsonString += "\"_value" + valuePart;

                for (const auto line : line.inlineValues) {
                    jsonString += line;
                }
            }  else {
                if (line.value == "{") {
                    valuePart = "\": " + line.value + "\n";
                } else {
                    valuePart = "\": \"" + line.value + "\",\n";
                }
                jsonString += "\"" + line.name + valuePart;
            }
            
        } 
        
        if (line.type == "op" && objects[i + 1].level < previous) {
            jsonString += "}";
        }


        if (i != objects.size() - 1) {
            if (line.type == "cl" && objects[i + 1].level < previous) {
                jsonString += "},\n";
            }
        } 

        if (line.level > highest) highest = line.level;
    }

    for (int i = 0; i < objects[objects.size() - 1].level; i++) {
        jsonString += "}";
    }

    return escapeOutput(jsonString);
}

int main()
{

    std::ifstream input("logs\\PaulMondl_DESKTOP-SV73V84\\2025-10-22_17-36-20_System.json");
    std::string line;

    while(std::getline(input, line)) {
        int start = line.find("<Event");
        int end = line.find("/Event>");
        std::cout << line.substr(start, end - 1) << std::endl << std::endl;

        std::string rawData = line.substr(start, end - 1);

        std::vector<Entry> collection = findKeys(rawData);

        std::string json = buildString(collection);

        std::cout << json << std::endl;
    }
    
    return 0;
}