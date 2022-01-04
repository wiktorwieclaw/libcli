```c++
struct WindowSettings {
    bool is_fullscreen = false;
    int width = 800;
    int height = 600;
};

enum class Scenery { Plains, Forest };

// enables Scenery objects to bind to libcli::cli
std::istream& operator>>(std::istream& is, Scenery& out) {
    std::string str;
    is >> str;
    const auto lc = to_lowercase(str);
    if (lc == "plains") { out = Scenery::Plains; }
    else if (lc == "forest") { out = Scenery::Forest; }
    else { is.setstate(std::ios_base::failbit); }
    return is;
}

int main(int argc, char** argv) {
    WindowSettings settings;
    Scenery scenery;
    
    libcli::cli cli;
    cli.add_option(settings.is_fullscreen, "--fullscreen", "-f");
    cli.add_option(settings.width, "--width", "-w");
    cli.add_option(settings.height, "--height", "-h");
    cli.add_argument(scenery);
    
    try {
        cli.parse(argc, argv);
    }
    catch (const libcli::invalid_program_arguments& ex) {
        std::cout << ex.what();
        return 1;
    }
}
```
