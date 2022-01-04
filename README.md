```c++
struct WindowSettings {
    bool is_fullscreen = false;
    int width = 800;
    int height = 600;
};

enum class Scenery { Plains, Forest };

// enables Scenery objects to bind to libcli::cli
void parse(std::string_view in, Scenery& out) {
    const auto lc_in = to_lowercase(in);
    if (lc_in == "plains") { out = Scenery::Plains; }
    else if (lc_in == "forest") { out = Scenery::Forest; }
    else {
        throw libcli::invalid_program_arguments{
            std::string{in} + "is not a valid scenario"s
        };
    }
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
