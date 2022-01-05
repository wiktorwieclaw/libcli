# libcli
Lightweight C++20 program arguments parser.

## Examples
### Basic usage
```c++
int main(int argc, char** argv) {
    libcli::cli cli;

    bool flag; 
    cli.add_option(flag, "--flag", "-f");
    
    std::string str1; 
    cli.add_option(str1, "--option", "-o");

    // std::optional can be used for types with no proper sentinel value
    std::optional<int> num;
    cli.add_option(num, "--number", "-n");

    std::string str2; 
    cli.add_argument(str2);

    // multi-arguments can be defined using libcli::multi tag
    std::vector<std::string> sources;
    cli.add_argument(libcli::multi, sources);

    try {
        cli.parse(argc, argv);
    }
    catch (const libcli::parsing_error& ex) {
        std::cout << ex.what();
        return 1;
    }

    if (flag) { /* ... */ }
    if (!str1.empty()) { /* ... */ }
    if (num) { /* ... */ }
}
```

### User defined types
```c++
enum class Scenery { Plains, Forest };

// enables Scenery objects to bind to libcli::cli
std::istream& operator>>(std::istream& is, Scenery& out) {
    const std::string str(std::istreambuf_iterator<char>(is), {});
    const auto lc = to_lowercase(str);
    if (lc == "plains") { out = Scenery::Plains; }
    else if (lc == "forest") { out = Scenery::Forest; }
    else { is.setstate(std::ios_base::failbit); }
    return is;
}

int main(int argc, char** argv) {
    Scenery scenery;
    
    libcli::cli cli;
    cli.add_argument(scenery);
}
```
