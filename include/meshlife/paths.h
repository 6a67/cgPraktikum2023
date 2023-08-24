#include <filesystem>
#include <iostream>

extern std::filesystem::path assets_path;
extern std::filesystem::path shaders_path;

inline void init_paths()
{
    shaders_path = std::filesystem::current_path() / "shaders";
    if (!std::filesystem::exists(shaders_path))
    {
        shaders_path = std::filesystem::current_path() / ".." / "src" / "shaders";
        std::cout << "Could not find shaders folder in current directory! Using "
                  << std::filesystem::absolute(shaders_path) << std::endl;
    }
    else
    {
        std::cout << "Found shaders folder in current directory! Using " << std::filesystem::absolute(shaders_path)
                  << std::endl;
    }

    assets_path = std::filesystem::current_path() / "assets";
    if (!std::filesystem::exists(assets_path))
    {
        assets_path = std::filesystem::current_path() / ".." / "assets";
        std::cout << "Could not find assets folder in current directory! Using "
                  << std::filesystem::absolute(assets_path) << std::endl;
    }
    else
    {
        std::cout << "Found assets folder in current directory! Using " << std::filesystem::absolute(assets_path)
                  << std::endl;
    }
}
