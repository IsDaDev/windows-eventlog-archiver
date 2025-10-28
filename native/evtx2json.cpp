#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wrange-loop-construct"

#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <fstream>
#include <chrono>

struct Entry{
    int id;
    int level;
    std::string name;
    std::string type;
    std::string directParent;
    std::vector<std::string> inlineValues;
    std::string value;
};

std::string escapeOutput(std::string input)
{
    std::regex leadingCommaAfterBrace(R"(\{,)");
    std::regex backslash(R"(\\)");
    std::regex trailingCommaPattern(",(\\s*})");
    std::regex newLine("\n");

    input = std::regex_replace(input, trailingCommaPattern, "$1");
    input = std::regex_replace(input, newLine, "$1");
    input = std::regex_replace(input, backslash, "\\");
    input = std::regex_replace(input, leadingCommaAfterBrace, "{");

    return input;
}

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

    modified += input.replace(delimiter, 1, "\":\"");  // replace '=' with "':"

    if (!modified.empty() && modified.back() == '/') {
        modified = modified.substr(0, modified.size() - 1);
    }

    modified += "\"";

    return modified;
}

std::vector<std::string> fetchInlineValues(std::string currentKey) 
{
    std::vector<std::string> values;
    std::vector<std::string> returnV;
    std::string tmp;
    bool open = false;

    for (const char chr : currentKey) {
            if (chr == ' ' && !open) {
            if (tmp.size() > 1) {
                values.push_back(tmp);
                tmp.clear();
            }
        } else if (chr == '\'' && !open) {
            open = true;
        } else if (chr == '\'' && open) {
            values.push_back(tmp);
            tmp.clear();
            open = false;
        } else {
            tmp += chr;
        }
    }

    if (tmp.size() > 1) {
        values.push_back(tmp);
    }

    for (int i = 2; i < values.size(); i++) {
        std::string value = values[i];
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
        char identifier = element[0];
        
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
            } else if (collection[i + 1][0] == 'c') {
                value = "";
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

    for (int i = 0; i < finding.size(); i++) {
        if (finding[i].type == "cl") {
            if (i > 0 && finding[i - 1].value == "{") {
                finding[i - 1].value = "";
            }
        }
    }

    return finding;
}

std::string findSameParams(std::string input) 
{
    int pos1 = input.find(':');

    if (pos1 == std::string::npos) return "";

    return input.substr(0, pos1);
}

std::string buildString(std::vector<Entry> objects) 
{
    std::string jsonString;
    int squareBrackets = 0;
    int highest = 0;

    for (int i = 0; i < objects.size(); i++) {
        Entry line = objects[i];


        if (line.id == 0) {
            jsonString += "{\"" + line.name + "\": " + line.value + line.inlineValues[0] + ", ";
            continue;
        } 

        int previous = objects[i - 1].level;

        if (line.type == "sc") {
            if (line.inlineValues.empty()) {
                jsonString += "\"" + line.name.substr(0, line.name.size() - 1) + "\": \"\",";
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
            if (line.inlineValues.size() > 0 && line.value != "" && line.value != "{" || line.inlineValues.size() == 1 && line.value == "") {
                if (line.inlineValues.size() == 1) {
                    // Check if we can safely access objects[i + 2]
                    if (i + 2 < objects.size()) {
                        Entry next = objects[i + 2];
                        
                        // Check if we can safely access objects[i - 2]
                        if (i >= 2) {
                            Entry last = objects[i - 2];

                            if (next.inlineValues.size() != 0 && findSameParams(line.inlineValues[0]) == findSameParams(next.inlineValues[0])) {
                                if (jsonString[jsonString.size() - 2] == '{') jsonString = jsonString.substr(0, jsonString.size() - 2) + "[";
                                jsonString += "{" + line.inlineValues[0] + ",\"_Value\":\"" + line.value + "\"},"; 
                            } else if (!last.inlineValues.empty() && findSameParams(line.inlineValues[0]) == findSameParams(last.inlineValues[0])) {
                                jsonString += "{" + line.inlineValues[0] + ",\"_Value\":\"" + line.value + "\"}],"; 
                                squareBrackets++;
                            } else {
                                jsonString += "\"" + line.name + "\": {";
                                jsonString += "\"_Value\":\"" + line.value + "\"";
                                if (!line.inlineValues.empty()) {
                                    jsonString += ",";
                                    for (int j = 0; j < line.inlineValues.size(); j++) {
                                        if (j != line.inlineValues.size() - 1) {
                                            jsonString += line.inlineValues[j] + ",";
                                        } else {
                                            jsonString += line.inlineValues[j];
                                        }
                                    }
                                }
                                jsonString += "},";
                            }
                        }
                    }
                }           
            } else {
                std::string firstPart = "\"" + line.name;
                if (line.value == "{") {
                    valuePart = firstPart + "\": " + line.value + "\n";
                } else {
                    if (i >= 2 && i + 2 < objects.size() & !line.inlineValues.empty()) {
                        Entry last = objects[i - 2];
                        Entry next = objects[i + 2];
                        if (next.inlineValues.size() != 0 && findSameParams(last.inlineValues[0]) == findSameParams(next.inlineValues[0]) && findSameParams(next.inlineValues[0]) != findSameParams(line.inlineValues[0])) {
                            valuePart.clear();
                            continue;
                        } 
                    } else {
                        valuePart = firstPart + "\": \"" + line.value + "\",\n";
                    }
                }
                jsonString += valuePart;
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

    int length = objects[objects.size() - 1].level;

    for (int i = 0; i < length - squareBrackets; i++) {
        jsonString += "}";
    }

    return escapeOutput(jsonString);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " \"path-to-input\" \"path-to-output\"" << std::endl;
        return -1;
    }

    auto start = std::chrono::high_resolution_clock::now();

    std::ifstream input(argv[1]);
    std::ofstream output(argv[2]);
    std::string line;

    while(std::getline(input, line)) {
        int start = line.find("<Event");
        int end = line.find("/Event>");
        std::string rawData = line.substr(start, end - 1);
    
        std::vector<Entry> collection = findKeys(rawData);
        std::string json = buildString(collection);
        output << json << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration < double > duration = end - start;

    std::cout << "Converted file " << argv[1] << " in " << duration.count() << " seconds" << std::endl;

    return 0;
}
