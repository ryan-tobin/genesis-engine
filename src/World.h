#pragma once

#include <vector>
#include <SFML/Graphics.hpp>

enum class TerrainType
{
    DEEP_WATER,
    SHALLOW_WATER,
    SAND,
    GRASS,
    FOREST,
    ROCK,
    SNOW
};

struct TerrainColor
{
    static sf::Color getColor(TerrainType type)
    {
        switch (type)
        {
        case TerrainType::DEEP_WATER:
            return sf::Color(0, 50, 120);
        case TerrainType::SHALLOW_WATER:
            return sf::Color(20, 100, 180);
        case TerrainType::SAND:
            return sf::Color(238, 203, 173);
        case TerrainType::GRASS:
            return sf::Color(86, 152, 23);
        case TerrainType::FOREST:
            return sf::Color(34, 100, 34);
        case TerrainType::ROCK:
            return sf::Color(130, 130, 130);
        case TerrainType::SNOW:
            return sf::Color(255, 255, 255);
        }
        return sf::Color::Black;
    }
};

class World
{
public:
    enum class IslandMode
    {
        SINGLE,
        ARCHIPELAGO
    };

private:
    int width;
    int height;
    int tileSize;
    int seed;
    IslandMode islandMode = IslandMode::SINGLE;

    std::vector<std::vector<float>> elevationMap;
    std::vector<std::vector<TerrainType>> terrainTypes;

    // Noise parameters
    float frequency = 0.005f; // Lower frequency for larger features
    float lacunarity = 2.0f;
    float persistence = 0.5f;
    int octaves = 6;

    // Island parameters
    float islandFalloffA = 3.0f;
    float islandFalloffB = 2.2f;

    // Terrain thresholds
    struct TerrainThresholds
    {
        float deepWater = -0.5f;
        float shallowWater = -0.1f;
        float sand = 0.0f;
        float grass = 0.15f;
        float forest = 0.35f;
        float rock = 0.6f;
        float snow = 0.8f;
    } thresholds;

    // Helper functions
    float generateOctaveNoise(float x, float y);
    float calculateFalloff(float x, float y);
    float calculateArchipelagoFalloff(float x, float y);
    void applyFalloffMap();
    TerrainType getTerrainType(float elevation);

public:
    World(int width, int height, int tileSize, int seed);

    void generateNoiseMap();
    void assignTerrainTypes();
    void render(sf::RenderWindow &window);
    void setIslandMode(IslandMode mode) { islandMode = mode; }

    // Getters
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    float getElevation(int x, int y) const;
    TerrainType getTerrain(int x, int y) const;
};