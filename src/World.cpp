#include "World.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <iostream>

// Simple hash function for procedural generation
static float hash(int x, int y, int seed)
{
    int n = x + y * 57 + seed * 131;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

// Smooth interpolation
static float smoothstep(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

// 2D Perlin-style noise implementation
static float noise2D(float x, float y, int seed)
{
    int xi = (int)std::floor(x);
    int yi = (int)std::floor(y);

    float xf = x - xi;
    float yf = y - yi;

    // Get corner values
    float v00 = hash(xi, yi, seed);
    float v10 = hash(xi + 1, yi, seed);
    float v01 = hash(xi, yi + 1, seed);
    float v11 = hash(xi + 1, yi + 1, seed);

    // Interpolate
    float sx = smoothstep(xf);
    float sy = smoothstep(yf);

    float a = v00 * (1.0f - sx) + v10 * sx;
    float b = v01 * (1.0f - sx) + v11 * sx;

    return a * (1.0f - sy) + b * sy;
}

World::World(int width, int height, int tileSize, int seed)
    : width(width), height(height), tileSize(tileSize), seed(seed)
{

    // Initialize elevation map
    elevationMap.resize(height, std::vector<float>(width, 0.0f));
    terrainTypes.resize(height, std::vector<TerrainType>(width, TerrainType::DEEP_WATER));
}

float World::generateOctaveNoise(float x, float y)
{
    float value = 0.0f;
    float amplitude = 1.0f;
    float freq = frequency;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++)
    {
        value += noise2D(x * freq, y * freq, seed + i) * amplitude;
        maxValue += amplitude;

        amplitude *= persistence;
        freq *= lacunarity;
    }

    // Normalize to [-1, 1]
    return value / maxValue;
}

float World::calculateFalloff(float x, float y)
{
    // Normalize coordinates to [-1, 1]
    float nx = (x / (float)width) * 2.0f - 1.0f;
    float ny = (y / (float)height) * 2.0f - 1.0f;

    // Use the maximum of x and y distance for a square-ish island
    float distance = std::max(std::abs(nx), std::abs(ny));

    // For a more circular island, use Euclidean distance instead:
    // float distance = std::sqrt(nx * nx + ny * ny);

    // Apply smooth falloff curve
    float value = std::max(0.0f, 1.0f - distance);
    return std::pow(value, islandFalloffA) /
           (std::pow(value, islandFalloffA) +
            std::pow(islandFalloffB - islandFalloffB * value, islandFalloffA));
}

float World::calculateArchipelagoFalloff(float x, float y)
{
    float nx = (x / (float)width) * 2.0f - 1.0f;
    float ny = (y / (float)height) * 2.0f - 1.0f;

    // Multiple island centers with different sizes and strengths
    // Main island
    float island1 = 1.0f - std::sqrt((nx - 0.3f) * (nx - 0.3f) + (ny - 0.2f) * (ny - 0.2f)) * 1.2f;

    // Secondary islands
    float island2 = 1.0f - std::sqrt((nx + 0.4f) * (nx + 0.4f) + (ny + 0.3f) * (ny + 0.3f)) * 1.5f;
    float island3 = 1.0f - std::sqrt((nx - 0.1f) * (nx - 0.1f) + (ny - 0.5f) * (ny - 0.5f)) * 1.8f;

    // Smaller islands
    float island4 = 1.0f - std::sqrt((nx + 0.6f) * (nx + 0.6f) + (ny - 0.4f) * (ny - 0.4f)) * 2.5f;
    float island5 = 1.0f - std::sqrt((nx - 0.7f) * (nx - 0.7f) + (ny + 0.5f) * (ny + 0.5f)) * 3.0f;

    // Combine islands with different weights
    float value = std::max({island1 * 0.9f,
                            island2 * 0.7f,
                            island3 * 0.6f,
                            island4 * 0.5f,
                            island5 * 0.4f,
                            0.0f});

    // Apply falloff curve
    return std::pow(value, islandFalloffA) /
           (std::pow(value, islandFalloffA) +
            std::pow(islandFalloffB - islandFalloffB * value, islandFalloffA));
}

void World::generateNoiseMap()
{
    // Generate base noise
    float minElev = 1.0f, maxElev = -1.0f;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            elevationMap[y][x] = generateOctaveNoise(x, y);
            minElev = std::min(minElev, elevationMap[y][x]);
            maxElev = std::max(maxElev, elevationMap[y][x]);
        }
    }

    std::cout << "Noise range before falloff: [" << minElev << ", " << maxElev << "]" << std::endl;

    // Apply island falloff
    applyFalloffMap();

    // Check final range
    minElev = 1.0f;
    maxElev = -1.0f;
    int landTiles = 0;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            minElev = std::min(minElev, elevationMap[y][x]);
            maxElev = std::max(maxElev, elevationMap[y][x]);
            if (elevationMap[y][x] > thresholds.sand)
                landTiles++;
        }
    }

    float landPercentage = (landTiles * 100.0f) / (width * height);
    std::cout << "Final elevation range: [" << minElev << ", " << maxElev << "]" << std::endl;
    std::cout << "Land coverage: " << landPercentage << "%" << std::endl;
}

void World::applyFalloffMap()
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float falloff = (islandMode == IslandMode::ARCHIPELAGO) ? calculateArchipelagoFalloff(x, y) : calculateFalloff(x, y);

            // Blend the noise with the falloff
            // The falloff should make edges go to water (-1) and center stay high
            elevationMap[y][x] = elevationMap[y][x] + falloff - 0.5f;

            // Clamp to valid range
            elevationMap[y][x] = std::max(-1.0f, std::min(1.0f, elevationMap[y][x]));
        }
    }
}

TerrainType World::getTerrainType(float elevation)
{
    if (elevation < thresholds.deepWater)
        return TerrainType::DEEP_WATER;
    if (elevation < thresholds.shallowWater)
        return TerrainType::SHALLOW_WATER;
    if (elevation < thresholds.sand)
        return TerrainType::SAND;
    if (elevation < thresholds.grass)
        return TerrainType::GRASS;
    if (elevation < thresholds.forest)
        return TerrainType::FOREST;
    if (elevation < thresholds.rock)
        return TerrainType::ROCK;
    return TerrainType::SNOW;
}

void World::assignTerrainTypes()
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            terrainTypes[y][x] = getTerrainType(elevationMap[y][x]);
        }
    }
}

void World::render(sf::RenderWindow &window)
{
    // Use vertex array for efficient rendering
    sf::VertexArray vertices(sf::PrimitiveType::Triangles, width * height * 6);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int index = (y * width + x) * 6;

            // Define quad corners
            float left = x * tileSize;
            float top = y * tileSize;
            float right = left + tileSize;
            float bottom = top + tileSize;

            // Get terrain color with slight variation for visual interest
            sf::Color baseColor = TerrainColor::getColor(terrainTypes[y][x]);

            // Add subtle noise to the color for texture
            int variation = (int)(hash(x, y, seed * 7) * 10) - 5;
            sf::Color color(
                std::max(0, std::min(255, baseColor.r + variation)),
                std::max(0, std::min(255, baseColor.g + variation)),
                std::max(0, std::min(255, baseColor.b + variation)));

            // Create two triangles for the quad
            // Triangle 1
            vertices[index + 0].position = sf::Vector2f(left, top);
            vertices[index + 1].position = sf::Vector2f(right, top);
            vertices[index + 2].position = sf::Vector2f(left, bottom);

            // Triangle 2
            vertices[index + 3].position = sf::Vector2f(right, top);
            vertices[index + 4].position = sf::Vector2f(right, bottom);
            vertices[index + 5].position = sf::Vector2f(left, bottom);

            // Set colors
            for (int i = 0; i < 6; i++)
            {
                vertices[index + i].color = color;
            }
        }
    }

    window.draw(vertices);
}

void World::renderHeightmap(sf::RenderWindow &window)
{
    // Render as grayscale heightmap
    sf::VertexArray vertices(sf::PrimitiveType::Triangles, width * height * 6);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int index = (y * width + x) * 6;

            // Define quad corners
            float left = x * tileSize;
            float top = y * tileSize;
            float right = left + tileSize;
            float bottom = top + tileSize;

            // Convert elevation to grayscale (0-255)
            float elevation = elevationMap[y][x];
            int gray = (int)((elevation + 1.0f) * 0.5f * 255.0f);
            gray = std::max(0, std::min(255, gray));

            sf::Color color(gray, gray, gray);

            // Create two triangles for the quad
            vertices[index + 0].position = sf::Vector2f(left, top);
            vertices[index + 1].position = sf::Vector2f(right, top);
            vertices[index + 2].position = sf::Vector2f(left, bottom);

            vertices[index + 3].position = sf::Vector2f(right, top);
            vertices[index + 4].position = sf::Vector2f(right, bottom);
            vertices[index + 5].position = sf::Vector2f(left, bottom);

            // Set colors
            for (int i = 0; i < 6; i++)
            {
                vertices[index + i].color = color;
            }
        }
    }

    window.draw(vertices);
}

float World::getElevation(int x, int y) const
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        return elevationMap[y][x];
    }
    return -1.0f; // Out of bounds
}

TerrainType World::getTerrain(int x, int y) const
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        return terrainTypes[y][x];
    }
    return TerrainType::DEEP_WATER; // Out of bounds
}

void World::modifyElevation(int x, int y, float delta)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        elevationMap[y][x] += delta;
        // Keep elevation in reasonable bounds
        elevationMap[y][x] = std::max(-1.0f, std::min(1.0f, elevationMap[y][x]));
    }
}

void World::normalizeElevation()
{
    // Find min and max elevation
    float minElev = elevationMap[0][0];
    float maxElev = elevationMap[0][0];

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            minElev = std::min(minElev, elevationMap[y][x]);
            maxElev = std::max(maxElev, elevationMap[y][x]);
        }
    }

    // Normalize to [-1, 1] range
    float range = maxElev - minElev;
    if (range > 0)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                elevationMap[y][x] = ((elevationMap[y][x] - minElev) / range) * 2.0f - 1.0f;
            }
        }
    }
}