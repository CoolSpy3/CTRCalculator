#include <algorithm>
#include <cmath>
#include <deque>
#include <iostream>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>

typedef long double number_t; // Internal Number Type
#define ston(...) static_cast<number_t>(std::stold(__VA_ARGS__)) // String to number_t conversion

std::deque<number_t> stack; // RPN Stack
std::map<std::string, std::string> aliases; // User defined aliases

#define CONFIG_FILE_NAME ".calcrc"

#pragma region getUserConfigDir() (Cross Platform Definition)
#ifdef _WIN32
#include <Shlobj.h>

std::filesystem::path getUserConfigDir() {
    if (TCHAR path[MAX_PATH]; SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_PROFILE, nullptr, 0, path))) {
        return path;
    }
    return "";
}

#else

// https://stackoverflow.com/a/2910392/22362115
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

std::filesystem::path getUserConfigDir() {
    if (const char *configDir = getenv("XDG_CONFIG_HOME"); configDir) {
        return configDir;
    }
    if (const char *configDir = getenv("HOME"); configDir) {
        return configDir;
    }
    return getpwuid(getuid())->pw_dir;
}

#endif
#pragma endregion

// Originally this was just a normal trim function, but then I realized that because *this is math*,
// whitespace is unnecessary for most functions, so I wrote an updated one that just deletes it.
// It's looking like that might make some more complex syntax difficult though, so this might get
// rolled back in the future.

// https://stackoverflow.com/a/217605/22362115
// inline std::string trim(std::string s) {
//     // ReSharper disable once CppUseRangeAlgorithm
//     s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](const unsigned char ch) {
//         return !std::isspace(ch);
//     }));
//     s.erase(std::find_if(s.rbegin(), s.rend(), [](const unsigned char ch) {
//         return !std::isspace(ch);
//     }).base(), s.end());
//     return s;
// }

inline std::string eraseWhitespace(std::string s) {
    erase_if(s, [](const unsigned char ch) {
        return std::isspace(ch);
    });
    return s;
}

/**
 * Reads a string starting with `openChar`, ignoring matching pairs of `openChar`, `closeChar`, until the matching
 * `closeChar`is found. It then returns a boolean indicating whether the function was successful. If it was, it updates
 * `expr` to be the expression enclosed within the parentheses and `rest` to be the text remaining after the matching
 * instance of `closeChar`.
 *
 * If the string does not begin with `openChar`, or a matching `closeChar` is not found, this function returns
 * `false` and does not update either of the strings.
 */
bool processParenthetical(const std::string& str, const char openChar, const char closeChar,
                          std::string& expr, std::string& rest) {
    if (str.front() != openChar)
        return false;

    size_t num_parens = 0;
    for (size_t i = 0; i < str.size(); i++) {
        const char c = str.at(i);
        if (c == openChar) num_parens++;
        else if (c == closeChar) num_parens--;

        if (num_parens == 0) {
            // Allow the user to pass the same value to str as to expr
            // Calculating `expr` here ensures that when we overwrite `rest`, we can still calculate `expr`
            const std::string expression = str.substr(1, i - 1);
            rest = str.substr(i + 1);
            expr = expression;
            return true;
        }
    }
    return false;
}

// Pops the top element from the stack and returns it.
number_t pop_back() {
    const number_t num = stack.back();
    stack.pop_back();
    return num;
}

// Possible results from calling execute(...)
enum ExecuteResult {
    EXECUTED = 0,
    ERR,
    END_ESCAPED, // Command is incomplete and ends with '\'
    EXIT_REQUESTED // Command requested program exit
};

ExecuteResult execute(std::string input) {
#define EXECUTE_SUBEXPRESSION(expr) if (const ExecuteResult res = execute(expr); res != EXECUTED) return res;
#define NOT_ENOUGH_ARGS goto err;
    try {
        static std::string lastInput;
        input = eraseWhitespace(std::move(lastInput) + input);
        if (const size_t commentPos = input.find('#'); commentPos != std::string::npos)
            input.erase(commentPos);

        if (input.empty())
            return EXECUTED;

        if (input.back() == '\\') {
            lastInput = input.substr(0, input.size() - 1);
            return END_ESCAPED;
        }

        // Add Alias ([key]=[value])
        if (const size_t eqPos = input.find('='); eqPos != std::string::npos) {
            const std::string alias = input.substr(0, eqPos);
            std::string value = input.substr(eqPos + 1);
            if (value.empty()) {
                aliases.erase(alias);
            } else if (value.front() == '(') {
                std::string rest;
                if (!processParenthetical(value, '(', ')', value, rest) ||
                        (!rest.empty() && rest.front() != ';')) // Invalid Syntax
                    goto err;
                EXECUTE_SUBEXPRESSION(std::format("{}={}", alias, value));
                if (!rest.empty())
                    return execute(rest.substr(1));
            } else if (value.front() == '{') {
                std::string rest;
                if (!processParenthetical(value, '{', '}', value, rest) ||
                        (!rest.empty() && rest.front() != ';')) // Invalid Syntax
                    goto err;
                EXECUTE_SUBEXPRESSION(value);
                if (stack.empty()) NOT_ENOUGH_ARGS;
                EXECUTE_SUBEXPRESSION(std::format("{}={}", alias, pop_back()));
                if (!rest.empty())
                    return execute(rest.substr(1));
            } else {
                aliases[alias] = value;
            }
            return EXECUTED;
        }

        // Apply Alias (if it exists)
        if (const auto alias = aliases.find(input); alias != aliases.end())
            return execute(alias->second);

        // Split Line
        if (const size_t sepPos = input.find(';'); sepPos != std::string::npos) {
            EXECUTE_SUBEXPRESSION(input.substr(0, sepPos));
            return execute(input.substr(sepPos + 1));
        }

        // Special case for [num] [+-*/] [num]
        try {
            size_t numConvertedChars1, numConvertedChars2;
            const auto num1 = ston(input, &numConvertedChars1);
            // If a number wasn't read or there aren't at least two more characters in the input
            if (numConvertedChars1 == 0 || numConvertedChars1 + 2 > input.length())
                goto not_basic_arithmatic;
            switch (input.at(numConvertedChars1)) {
                case '+':
                case '-':
                case '*':
                case '/':
                    break;
                default:
                    goto not_basic_arithmatic;
            }
            const auto num2 = ston(input.substr(numConvertedChars1 + 1), &numConvertedChars2);
            if (numConvertedChars2 != 0) {
                stack.push_back(num1);
                stack.push_back(num2);
                return execute(std::string(1, input.at(numConvertedChars1)));
            }
        } catch (...) {}
        not_basic_arithmatic:

#pragma region Standard Operations/Functions/Commands
#define unaryFunction(name) else if (input == "\\" # name) { \
if (stack.empty()) NOT_ENOUGH_ARGS; \
    stack.push_back(name##l(pop_back())); \
}
        if (input == "\\exit") {
            return EXIT_REQUESTED;
        } else if (input == "+") {
            if (stack.size() < 2) NOT_ENOUGH_ARGS;
            stack.push_back(pop_back() + pop_back());
        } else if (input == "-") {
            if (stack.size() < 2) NOT_ENOUGH_ARGS;
            stack.push_back(pop_back() - pop_back());
        } else if (input == "*") {
            if (stack.size() < 2) NOT_ENOUGH_ARGS;
            stack.push_back(pop_back() * pop_back());
        } else if (input == "/") {
            if (stack.size() < 2) NOT_ENOUGH_ARGS;
            const number_t num2 = pop_back();
            stack.push_back(pop_back() / num2);
        } else if (input == "\\pow") {
            if (stack.size() < 2) NOT_ENOUGH_ARGS;
            stack.push_back(powl(pop_back(), pop_back()));
        } else if (input == "1/") {
            if (stack.empty()) NOT_ENOUGH_ARGS;
            stack.push_back(1 / pop_back());
        } else if (input == "\\clear") {
            stack.clear();
        } else if (input == "\\swap") {
            if (stack.size() < 2) NOT_ENOUGH_ARGS;
            const number_t val = pop_back();
            stack.insert(stack.end() - 1, val);
        } else if (input == "\\roll") {
            if (stack.size() < 2) NOT_ENOUGH_ARGS;
            const number_t val = stack.front();
            stack.pop_front();
            stack.push_back(val);
        } else if (input == "\\drop") {
            if (stack.empty()) NOT_ENOUGH_ARGS;
            stack.pop_back();
        } else if (input.front() == '[' && input.back() == ']') { // Dereference index in stack
            EXECUTE_SUBEXPRESSION(input.substr(1, input.size() - 2));
            const auto reqIdx = pop_back();

            // This still works with negative numbers because of unsigned integer rollover
            // ReSharper disable once CppRedundantCastExpression
            auto idx = static_cast<size_t>(llroundl(reqIdx));
            if (reqIdx < 0) { // I.e. -1 maps to the most recently pushed value
                idx += stack.size();
            }
            if (idx >= stack.size()) NOT_ENOUGH_ARGS;
            stack.push_back(stack.at(idx));
            return EXECUTED;
        } else if (input == "!!") {
            if (stack.empty()) NOT_ENOUGH_ARGS;
            stack.push_back(stack.back());
        } else if (input == "\\aliases") {
            for (const auto&[alias, value]: aliases)
                std::cout << std::format("{}={}\n", alias, value);
        }
        unaryFunction(sqrt)
        unaryFunction(cbrt)
        unaryFunction(sin)
        unaryFunction(cos)
        unaryFunction(tan)
        unaryFunction(asin)
        unaryFunction(acos)
        unaryFunction(atan)
        unaryFunction(sinh)
        unaryFunction(cosh)
        unaryFunction(tanh)
        unaryFunction(asinh)
        unaryFunction(acosh)
        unaryFunction(atanh)
        unaryFunction(log)
        unaryFunction(log2)
        unaryFunction(log10)
        unaryFunction(exp)
        else { // Try to interpret the input as a number and add it to the stack
            size_t numConvertedChars;
            const auto num = ston(input, &numConvertedChars);
            if (numConvertedChars != input.size()) goto err; // Not a number
            stack.push_back(num);
        }

        return EXECUTED;

#undef unaryFunction
#pragma endregion

    } catch (...) {
        goto err;
    }

    err: // Invalid input
    if (input.front() != '\\') { // Try prepending a '\' in case the user forgot it
        return execute("\\" + input);
    }
    std::cout << "ERR!\n";
    return ERR;
#undef NOT_ENOUGH_ARGS
#undef EXECUTE_SUBEXPRESSION
}

int main() {
    // Read and execute the config file
    if (std::filesystem::path homeDir = getUserConfigDir(); !homeDir.empty()) {
        if (std::filesystem::path configFile = homeDir / CONFIG_FILE_NAME; std::filesystem::exists(configFile)) {
            std::ifstream file(configFile);
            std::string line;
            while (std::getline(file, line)) { // Execute every line in the config file
                switch (execute(line)) {
                    case ERR:
                        std::cout << "Error in config file! File execution aborted!\n";
                        break;
                    case EXIT_REQUESTED:
                        std::cout << "Config file requested exit.\n";
                        return 0;
                    default:
                        break;
                }
            }
        } else { // The config file doesn't exist :(  Create it!
            std::ofstream file(configFile);
            file << "# CTRCalculator Config\n";
        }
    } else {
        std::cerr << "Could not find config directory! Config file will not be loaded!\n";
    }

    // Read and execute input from cin
    ExecuteResult lastResult = EXECUTED;
    while (lastResult != EXIT_REQUESTED) {
        if (lastResult != END_ESCAPED) { // If the last line did not end with a '\', print out the current stack and a prompt
            if (stack.empty()) { // If the stack is empty, don't print a random blank line
                std::cout << "> ";
            } else {
                for (const auto num : stack)
                    std::cout << num << ' ';

                std::cout << "\n> ";
            }
        }
        std::string input;
        std::getline(std::cin, input);
        lastResult = execute(input);
        std::cout << "\n";
    }
    return 0;
}
