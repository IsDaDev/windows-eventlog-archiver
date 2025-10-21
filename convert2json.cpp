#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <regex>
#include <chrono>
#include <fstream>

std::vector<std::string> constructVec;
int bracketsOpened = 0;

void splitString(const std::string& input, char delimiter,
                 std::vector<std::string>& out)
{
    std::istringstream stream(input);
    std::string token;

    while (getline(stream, token, delimiter)) {
        out.push_back(token);
    }
}


std::vector<std::string> iterAttributes(std::string attr)
{
    std::vector<std::string> out;
    splitString(attr, ' ', out);
    return out;
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
    //  SystemTime="2025-10-20T13:35:38.8040065Z"/  ->  "SystemTime":"2025-10-20T13:35:38.8040065Z"
    int delimiter = input.find('=');
    std::string modified = "\"";

    if (delimiter == std::string::npos) {
        return input;
    }

    modified += input.replace(delimiter, 1, "\":");  // replace '=' with "':"

    if (!modified.empty() && modified.back() == '/') {
        modified = modified.substr(0, modified.size() - 1) + "}";
        bracketsOpened--;
    }

    return replaceSingleQuotes(modified);
}

void handleMultiAttr(std::vector<std::string> input)
{
    std::string jsonString;
    if (input.size() > 1)
    {
        for (int i = 0; i < input.size(); i++) {
            std::string element = input[i];
            
            if (i == 0) {
                bracketsOpened++;
                jsonString += "\"" + element + "\":{";
                continue;
            }

            std::string normalized = normalizeAttr(element);
            jsonString += normalized;

            if (i != input.size() - 1) {
                jsonString += ",";
            }
        }
    }

    std::string last = input[input.size() - 1];

    constructVec.push_back(jsonString + ",\n");
}

void handleKey(std::string key)
{
    if (key.back() == '/') {
        std::string insert = "\"" + key.substr(0, key.size() - 1) + "\" : {},\n";
        constructVec.push_back(insert);
    } else {
        constructVec.push_back("\"" + key + "\":");
    }
}

std::vector<std::string> splitVisibleNewlines(const std::string& input)
{
    std::vector<std::string> result;
    std::string temp;
    
    for (size_t i = 0; i < input.size(); ++i) {
        if (i + 1 < input.size() && input[i] == '\\' && input[i + 1] == 'n') {
            result.push_back(temp);
            temp.clear();
            i++;
        } else {
            temp += input[i];
        }
    }

    if (!temp.empty()) result.push_back(temp);
    return result;
}

std::string findKeys(std::string xmlStr)
{
    bracketsOpened++;

    std::string currentKey;
    std::string value;
    std::string lastOpened;
    bool lastElementWasKey = false;

    bool start = false;
    for (int i = 0; i < xmlStr.size(); i++) {
        char chr = xmlStr[i];
        
        if (chr == '<') {
            if (!value.empty()) {
                
                std::string lastKey = constructVec[constructVec.size() - 1];
            
                if (lastKey[lastKey.size() - 1] != ':') {
                    constructVec.push_back("\"value\":\"" + value + "\",\n");
                } else {
                    constructVec.push_back("\"" + value + "\",\n");
                }
            }

            start = true;
            continue;

        // closing bracket, so key ends
        } else if (chr == '>') {
            if (currentKey[0] == '/') {
                std::string closing = currentKey.substr(1);
                
                if (value.empty() && closing == lastOpened) {
                    constructVec.push_back("{},");
                }
                
                lastOpened.clear();
                value.clear();
            } else {
                std::vector<std::string> out = iterAttributes(currentKey);
                lastOpened = out[0];
                lastElementWasKey = true;
                    if (out.size() > 1) {
                        if (lastElementWasKey) {
                            constructVec.push_back("{");
                            bracketsOpened++;
                            lastElementWasKey = false;
                        }
                        handleMultiAttr(out);
                    } else {
                        handleKey(currentKey);
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
    
    constructVec.push_back("}");
    bracketsOpened--;

    return "";
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

int main() 
{
    auto start = std::chrono::high_resolution_clock::now();

    // std::ifstream inputFile("logs/PaulMondl_DESKTOP-SV73V84/2025-10-20_15-36-33_Security.json");

    // std::string read;
    // while (std::getline(inputFile, read)) {
        // std::cout << read << std::endl << std::endl;
        std::string read = "<Event xmlns='http://schemas.microsoft.com/win/2004/08/events/event'><System><Provider Name='Microsoft-Windows-Security-Auditing' Guid='{54849625-5478-4994-a5ba-3e3b0328c30d}'/><EventID>5038</EventID><Version>0</Version><Level>0</Level><Task>12290</Task><Opcode>0</Opcode><Keywords>0x8010000000000000</Keywords><TimeCreated SystemTime='2025-10-20T13:36:31.7100797Z'/><EventRecordID>2089320</EventRecordID><Correlation/><Execution ProcessID='4' ThreadID='23548'/><Channel>Security</Channel><Computer>DESKTOP-SV73V84</Computer><Security/></System><EventData><Data Name='param1'>\\Device\\HarddiskVolume3\\Program Files\\Sophos\\Sophos AMSI Protection\\SophosAmsiProvider.dll</Data></EventData></Event>";
        findKeys(read);

        std::string temp;

        for (const auto line : constructVec) {
            std::regex newline("\n");
            std::string visible = std::regex_replace(line, newline, "\\n\n");
            temp += visible;
        }

        std::vector<std::string> lines = splitVisibleNewlines(temp);

        std::string raw = "";

        for (const auto& line : lines) {
            bool is = line[1] == '{';
            if (is) {
                raw += line.substr(2);
                bracketsOpened--;
            } else {
                raw += line;
            }
        }

        std::cout << "Brackets: " << bracketsOpened << std::endl;
        for (int i = 1; i < bracketsOpened; i++) {
            raw += '}';
        }
        
        std::string cleaned = escapeOutput(raw);

        std::cout << std::endl << cleaned << std::endl << std::endl;
    // }
    
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Elapsed time: " << duration << " ms" << std::endl;

    return 0;
}

