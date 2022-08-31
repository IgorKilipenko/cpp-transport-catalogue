#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

size_t TrimStart(std::string_view& str, const char ch = ' ') {
    size_t idx = str.find_first_not_of(ch);
    if (idx != std::string::npos) {
        str.remove_prefix(idx);
        return idx;
    }
    return 0;
}

size_t TrimEnd(std::string_view& str, const char ch = ' ') {
    size_t idx = str.find_last_not_of(ch);
    if (idx != str.npos) {
        str.remove_suffix(str.size() - idx - 1);
        return idx;
    }
    return 0;
}

std::string Trim(std::string&& str, const char ch = ' ') {
    std::string_view result{std::move(str)};
    TrimStart(result, ch);
    TrimEnd(result, ch);
    return std::string(std::move(result));
}

std::optional<std::filesystem::path> FindIncludeFile(const std::filesystem::path& filename, const std::filesystem::path& catalog) {
    if (std::filesystem::exists(catalog)) {
        for (const std::filesystem::path& c : std::filesystem::directory_iterator(catalog)) {
            std::filesystem::file_status status = std::filesystem::status(c);
            if (status.type() == std::filesystem::file_type::regular) {
                if (c.filename() == filename.filename()) {
                    return c;
                }
            } else {
                return FindIncludeFile(filename, c);
            }
        }
    }
    return nullopt;
}

std::optional<std::filesystem::path> FindForAbsPath(const std::smatch& m, const std::vector<std::filesystem::path>& include_directories) {
    for (const std::filesystem::path& directory : include_directories) {
        const std::filesystem::path pt(Trim(m[1].str()), std::filesystem::path::generic_format);
        const std::optional<std::filesystem::path> path = FindIncludeFile(pt, directory);
        if (path) {
            return path;
        }
    }
    return nullopt;
}

std::optional<std::filesystem::path> FindForRelPath(const std::filesystem::path& in_file, const std::smatch& m) {
    const std::filesystem::path path = in_file.parent_path() / m[1].str();
    if (std::filesystem::exists(path)) {
        return path;
    }
    return nullopt;
}

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {
    std::ifstream in(in_file);

    if (!in.is_open()) {
        std::cerr << in_file << std::endl;
        return false;
    }

    std::ofstream out(out_file, std::ios::app);
    if (!out.is_open()) {
        std::cerr << out_file << std::endl;
        return false;
    }

    static std::regex relatively(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    static std::regex absolutely(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    std::smatch m;
    std::string line;
    unsigned int row = 0;

    while (getline(in, line)) {
        row++;
        if (std::regex_match(line, m, relatively)) {
            std::optional<std::filesystem::path> include_path = FindForRelPath(in_file, m);

            if (!include_path) {
                include_path = FindForAbsPath(m, include_directories);
                if (!include_path) {
                    std::filesystem::path file = std::string(m[1]);
                    std::cout << "unknown include file "sv << file.string() << " at file "sv << in_file.string() << " at line "sv << row << std::endl;
                    return false;
                }
            }
            if (!Preprocess(*include_path, out_file, include_directories)) {
                return false;
            }
        } else if (std::regex_match(line, m, absolutely)) {
            std::optional<std::filesystem::path> include_path = FindForAbsPath(m, include_directories);
            if (!include_path) {
                std::filesystem::path file = std::string(m[1]);
                std::cout << "unknown include file "sv << file.string() << " at file "sv << in_file.string() << " at line "sv << row << std::endl;
                return false;
            }
            if (!Preprocess(*include_path, out_file, include_directories)) {
                return false;
            }
        } else {
            out << line << std::endl;
        }
    }
    return true;
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"sv;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"sv;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"sv;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"sv;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"sv;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"sv;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p, {"sources"_p / "include1"_p, "sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"sv;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

/*
int main() {
    Test();
}
*/