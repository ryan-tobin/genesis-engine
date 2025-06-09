#include "Climate.h"
#include "World.h"
#include <cmath>
#include <algorithm>
#include <queue>
#include <iostream>

ClimateSystem::ClimateSystem(int width, int height) : width(width), height(height)
{
    temperatureMap.resize(height, std::vector<float>(width, 0.0f));
    moistureMap.resize(height, std::vector<float>(width, 0.0f));
    biomeMap.resize(height, std::vector<BiomeType>(width, BiomeType::OCEAN));
}

void ClimateSystem::generateClimate(World &world)
{
    std::cout << "Generating climate..." << std::endl;

    for (int y = 0; y < height; y++)
    {
        float latitude = (float)y / height;

        for (int x = 0; x < width; x++)
        {
            float elevation = world.getElevation(x, y);
            temperatureMap[y][x] = calculateTemperature(elevation, latitude);
        }
    }

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            moistureMap[y][x] = calculateMoisture(world, x, y);
        }
    }

    smoothMoisture();
    smoothMoisture();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float elevation = world.getElevation(x, y);
            float temperature = temperatureMap[y][x];
            float moisture = moistureMap[y][x];

            biomeMap[y][x] = determineBiome(elevation, temperature, moisture);
        }
    }

    std::cout << "Climate generation complete!" << std::endl;
}

float ClimateSystem::calculateTemperature(float elevation, float latitude)
{
    float latitudeEffect = std::abs(latitude - 0.5f) * 2.0f;
    float baseTemp = baseTemperature - (latitudeTemperatureRange * latitudeEffect);

    float elevationInMeters = std::max(0.0f, elevation * 2000.0f);
    float tempDrop = (elevationInMeters / 1000.0f) * temperatureLapseRate;

    return baseTemp - tempDrop;
}

float ClimateSystem::calculateMoisture(const World &world, int x, int y)
{
    const int searchRadius = 20;
    float minDistance = searchRadius;

    for (int dy = -searchRadius; dy <= searchRadius; dy++)
    {
        for (int dx = -searchRadius; dx <= searchRadius; dx++)
        {
            int nx = x + dx;
            int ny = y + dy;

            if (nx >= 0 && nx < width && ny >= 0 && ny < height)
            {
                if (world.getElevation(nx, ny) < 0.0f)
                {
                    float distance = std::sqrt(dx * dx + dy * dy);
                    minDistance = std::min(minDistance, distance);
                }
            }
        }
    }

    float moisture = 1.0f - (minDistance / searchRadius);

    float elevation = world.getElevation(x, y);
    if (elevation > 0.5f)
    {
        moisture *= (1.0f - (elevation - 0.5f));
    }

    return std::max(0.0f, std::min(1.0f, moisture));
}

void ClimateSystem::smoothMoisture()
{
    std::vector<std::vector<float>> newMoisture = moistureMap;

    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            float sum = 0.0f;
            int count = 0;

            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dx = -1; dx <= 1; dx++)
                {
                    sum += moistureMap[y + dy][x + dx];
                    count++;
                }
            }
            newMoisture[y][x] = sum / count;
        }
    }

    moistureMap = newMoisture;
}

void ClimateSystem::generateRivers(World &world)
{
    const int numRivers = 20;

    for (int i = 0; i < numRivers; i++)
    {
        int startX = rand() % width;
        int startY = rand() % height;

        if (world.getElevation(startX, startY) < 0.4f)
            continue;

        int x = startX;
        int y = startY;
        int riverLength = 0;
        const int maxLength = 100;

        while (riverLength < maxLength)
        {
            int lowestX = x;
            int lowestY = y;
            float lowestElev = world.getElevation(x, y);

            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dx = -1; dx <= 1; dx++)
                {
                    if (dx == 0 && dy == 0)
                        continue;

                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx > 0 && nx < width && ny > 0 && ny < height)
                    {
                        float elev = world.getElevation(nx, ny);
                        if (elev < lowestElev)
                        {
                            lowestElev = elev;
                            lowestX = nx;
                            lowestY = ny;
                        }
                    }
                }
            }

            if (lowestX == x && lowestY == y)
                break;
            if (lowestElev < 0.0f)
                break;

            x = lowestX;
            y = lowestY;

            moistureMap[y][x] = std::min(1.0f, moistureMap[y][x] + 0.5f);

            for (int dy = -2; dy <= 2; dy++)
            {
                for (int dx = -2; dx <= 2; dx++)
                {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                    {
                        float distance = std::sqrt(dx * dx + dy * dy);
                        float moistureBonus = 0.3f * (1.0f - distance / 2.0f);
                        moistureMap[ny][nx] = std::min(1.0f, moistureMap[ny][nx] + moistureBonus);
                    }
                }
            }

            riverLength++;
        }
    }
}

BiomeType ClimateSystem::determineBiome(float elevation, float temperature, float moisture)
{
    if (elevation < -0.1f)
        return BiomeType::OCEAN;
    if (elevation < 0.0f)
        return BiomeType::BEACH;

    if (temperature < -5.0f)
        return BiomeType::ICE;

    if (temperature < 0.0f)
    {
        return BiomeType::TUNDRA;
    }
    else if (temperature < 10.0f)
    {
        if (moisture > 0.5f)
            return BiomeType::TAIGA;
        else
            return BiomeType::TUNDRA;
    }
    else if (temperature < 20.0f)
    {
        if (moisture > 0.6f)
            return BiomeType::TEMPERATE_FOREST;
        else if (moisture > 0.3f)
            return BiomeType::TEMPERATE_GRASSLAND;
        else
            return BiomeType::DESERT;
    }
    else
    {
        if (moisture > 0.7f)
            return BiomeType::TROPICAL_FOREST;
        else if (moisture > 0.3f)
            return BiomeType::SAVANNA;
        else
            return BiomeType::DESERT;
    }
}

void ClimateSystem::render(sf::RenderWindow &window, int tileSize)
{
    sf::VertexArray vertices(sf::PrimitiveType::Triangles, width * height * 6);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int index = (y * width + x) * 6;

            float left = x * tileSize;
            float top = y * tileSize;
            float right = left + tileSize;
            float bottom = top + tileSize;

            sf::Color color = BiomeColor::getColor(biomeMap[y][x]);

            vertices[index + 0].position = sf::Vector2f(left, top);
            vertices[index + 1].position = sf::Vector2f(right, top);
            vertices[index + 2].position = sf::Vector2f(left, bottom);

            vertices[index + 3].position = sf::Vector2f(right, top);
            vertices[index + 4].position = sf::Vector2f(right, bottom);
            vertices[index + 5].position = sf::Vector2f(left, bottom);

            for (int i = 0; i < 6; i++)
            {
                vertices[index + i].color = color;
            }
        }
    }

    window.draw(vertices);
}

void ClimateSystem::renderTemperature(sf::RenderWindow &window, int tileSize)
{
    sf::VertexArray vertices(sf::PrimitiveType::Triangles, width * height * 6);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int index = (y * width + x) * 6;

            float left = x * tileSize;
            float top = y * tileSize;
            float right = left + tileSize;
            float bottom = top + tileSize;

            // Temperature to color (blue = cold, red = hot)
            float temp = temperatureMap[y][x];
            float normalized = (temp + 10.0f) / 40.0f; // Normalize -10 to 30 range
            normalized = std::max(0.0f, std::min(1.0f, normalized));

            sf::Color color;
            if (normalized < 0.5f)
            {
                // Cold: Blue to white
                float t = normalized * 2.0f;
                color.r = 50 + (205 * t);
                color.g = 50 + (205 * t);
                color.b = 255;
            }
            else
            {
                // Warm: White to red
                float t = (normalized - 0.5f) * 2.0f;
                color.r = 255;
                color.g = 255 - (205 * t);
                color.b = 255 - (205 * t);
            }

            // Create triangles
            vertices[index + 0].position = sf::Vector2f(left, top);
            vertices[index + 1].position = sf::Vector2f(right, top);
            vertices[index + 2].position = sf::Vector2f(left, bottom);

            vertices[index + 3].position = sf::Vector2f(right, top);
            vertices[index + 4].position = sf::Vector2f(right, bottom);
            vertices[index + 5].position = sf::Vector2f(left, bottom);

            for (int i = 0; i < 6; i++)
            {
                vertices[index + i].color = color;
            }
        }
    }

    window.draw(vertices);
}

void ClimateSystem::renderMoisture(sf::RenderWindow &window, int tileSize)
{
    sf::VertexArray vertices(sf::PrimitiveType::Triangles, width * height * 6);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int index = (y * width + x) * 6;

            float left = x * tileSize;
            float top = y * tileSize;
            float right = left + tileSize;
            float bottom = top + tileSize;

            // Moisture to color (brown = dry, blue = wet)
            float moisture = moistureMap[y][x];
            sf::Color color;
            color.r = 139 * (1.0f - moisture);
            color.g = 90 * (1.0f - moisture) + 90 * moisture;
            color.b = 50 + 205 * moisture;

            // Create triangles
            vertices[index + 0].position = sf::Vector2f(left, top);
            vertices[index + 1].position = sf::Vector2f(right, top);
            vertices[index + 2].position = sf::Vector2f(left, bottom);

            vertices[index + 3].position = sf::Vector2f(right, top);
            vertices[index + 4].position = sf::Vector2f(right, bottom);
            vertices[index + 5].position = sf::Vector2f(left, bottom);

            for (int i = 0; i < 6; i++)
            {
                vertices[index + i].color = color;
            }
        }
    }

    window.draw(vertices);
}

float ClimateSystem::getTemperature(int x, int y) const
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        return temperatureMap[y][x];
    }
    return 0.0f;
}

float ClimateSystem::getMoisture(int x, int y) const
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        return moistureMap[y][x];
    }
    return 0.0f;
}

BiomeType ClimateSystem::getBiome(int x, int y) const
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        return biomeMap[y][x];
    }
    return BiomeType::OCEAN;
}