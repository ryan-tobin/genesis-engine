#pragma once

#include <vector>
#include <SFML/Graphics.hpp>

class World;

enum class BiomeType
{
    OCEAN,
    ICE,
    TUNDRA,
    TAIGA,
    TEMPERATE_FOREST,
    TEMPERATE_GRASSLAND,
    DESERT,
    SAVANNA,
    TROPICAL_FOREST,
    BEACH,
    LAKE,
    RIVER
};

struct BiomeColor
{
    static sf::Color getColor(BiomeType type)
    {
        switch (type)
        {
        case BiomeType::OCEAN:
            return sf::Color(0, 50, 120);
        case BiomeType::ICE:
            return sf::Color(240, 248, 255);
        case BiomeType::TUNDRA:
            return sf::Color(196, 204, 187);
        case BiomeType::TAIGA:
            return sf::Color(0, 100, 0);
        case BiomeType::TEMPERATE_FOREST:
            return sf::Color(34, 139, 34);
        case BiomeType::TEMPERATE_GRASSLAND:
            return sf::Color(154, 205, 50);
        case BiomeType::DESERT:
            return sf::Color(238, 203, 173);
        case BiomeType::SAVANNA:
            return sf::Color(209, 186, 116);
        case BiomeType::TROPICAL_FOREST:
            return sf::Color(0, 128, 0);
        case BiomeType::BEACH:
            return sf::Color(238, 214, 175);
        case BiomeType::LAKE:
            return sf::Color(100, 149, 237);
        case BiomeType::RIVER:
            return sf::Color(65, 105, 225);
        }
        return sf::Color::Black;
    }

    static const char *getName(BiomeType type)
    {
        switch (type)
        {
        case BiomeType::OCEAN:
            return "Ocean";
        case BiomeType::ICE:
            return "Ice";
        case BiomeType::TUNDRA:
            return "Tundra";
        case BiomeType::TAIGA:
            return "Taiga";
        case BiomeType::TEMPERATE_FOREST:
            return "Temperate Forest";
        case BiomeType::TEMPERATE_GRASSLAND:
            return "Temperate Grassland";
        case BiomeType::DESERT:
            return "Desert";
        case BiomeType::SAVANNA:
            return "Savanna";
        case BiomeType::TROPICAL_FOREST:
            return "Tropical Forest";
        case BiomeType::BEACH:
            return "Beach";
        case BiomeType::LAKE:
            return "Lake";
        case BiomeType::RIVER:
            return "River";
        }
        return "Unknown";
    }
};

class ClimateSystem
{
private:
    int width;
    int height;

    std::vector<std::vector<float>> temperatureMap;
    std::vector<std::vector<float>> moistureMap;
    std::vector<std::vector<BiomeType>> biomeMap;

    float baseTemperature = 20.0f;
    float temperatureLapseRate = 6.5f;
    float latitudeTemperatureRange = 30.0f;

    float calculateTemperature(float elevation, float latitude);
    float calculateMoisture(const World &world, int x, int y);
    BiomeType determineBiome(float elevation, float temperature, float moisture);
    void generateRivers(World &world);
    void smoothMoisture();

public:
    ClimateSystem(int width, int height);

    void generateClimate(World &world);
    void render(sf::RenderWindow &window, int tileSize);
    void renderTemperature(sf::RenderWindow &window, int tileSize);
    void renderMoisture(sf::RenderWindow &window, int tileSize);

    // Getters
    float getTemperature(int x, int y) const;
    float getMoisture(int x, int y) const;
    BiomeType getBiome(int x, int y) const;
};