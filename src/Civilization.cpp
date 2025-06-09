#include "Civilization.h"
#include "World.h"
#include "Climate.h"
#include <cmath>
#include <algorithm>
#include <queue>
#include <random>
#include <iostream>
#include <set>
#include <map>

CivilizationSystem::CivilizationSystem(int width, int height)
    : width(width), height(height), currentYear(0)
{

    territoryMap.resize(height, std::vector<int>(width, -1));
    developmentMap.resize(height, std::vector<float>(width, 0.0f));
    movementCost.resize(height, std::vector<float>(width, 1.0f));
}

void CivilizationSystem::initialize(const World &world, const ClimateSystem &climate)
{
    std::cout << "Initializing civilization..." << std::endl;

    // Calculate movement costs based on terrain
    calculateMovementCosts(world, climate);

    // Place initial cities
    placeInitialCities(world, climate);

    // Connect cities with roads
    connectCities();

    std::cout << "Civilization initialized with " << cities.size() << " cities!" << std::endl;
}

void CivilizationSystem::calculateMovementCosts(const World &world, const ClimateSystem &climate)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float elevation = world.getElevation(x, y);
            BiomeType biome = climate.getBiome(x, y);

            // Base cost on terrain type
            if (elevation < 0.0f)
            {
                movementCost[y][x] = 999.0f; // Can't build roads on water
            }
            else
            {
                float cost = 1.0f;

                // Elevation penalty
                cost += elevation * 3.0f;

                // Biome modifiers
                switch (biome)
                {
                case BiomeType::DESERT:
                case BiomeType::ICE:
                case BiomeType::TUNDRA:
                    cost += 2.0f;
                    break;
                case BiomeType::TROPICAL_FOREST:
                case BiomeType::TEMPERATE_FOREST:
                case BiomeType::TAIGA:
                    cost += 1.5f;
                    break;
                case BiomeType::TEMPERATE_GRASSLAND:
                case BiomeType::SAVANNA:
                    cost += 0.5f;
                    break;
                default:
                    break;
                }

                movementCost[y][x] = cost;
            }
        }
    }
}

std::string CivilizationSystem::generateCityName()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<> prefixDist(0, namePrefix.size() - 1);
    std::uniform_int_distribution<> suffixDist(0, nameSuffix.size() - 1);

    return namePrefix[prefixDist(gen)] + " " + nameSuffix[suffixDist(gen)];
}

float CivilizationSystem::calculateSiteSuitability(const World &world, const ClimateSystem &climate, int x, int y)
{
    float suitability = 0.0f;

    // Must be on land
    float elevation = world.getElevation(x, y);
    if (elevation < 0.05f)
        return 0.0f;

    // Prefer flat to gently sloped land
    if (elevation < 0.3f)
    {
        suitability += 10.0f;
    }
    else if (elevation < 0.5f)
    {
        suitability += 5.0f;
    }

    // Check biome suitability
    BiomeType biome = climate.getBiome(x, y);
    switch (biome)
    {
    case BiomeType::TEMPERATE_GRASSLAND:
    case BiomeType::TEMPERATE_FOREST:
        suitability += 15.0f;
        break;
    case BiomeType::SAVANNA:
    case BiomeType::TROPICAL_FOREST:
        suitability += 10.0f;
        break;
    case BiomeType::TAIGA:
        suitability += 5.0f;
        break;
    case BiomeType::DESERT:
    case BiomeType::TUNDRA:
    case BiomeType::ICE:
        suitability -= 5.0f;
        break;
    default:
        break;
    }

    // Proximity to water is crucial
    bool nearWater = false;
    int waterDistance = 999;
    for (int dy = -5; dy <= 5; dy++)
    {
        for (int dx = -5; dx <= 5; dx++)
        {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < width && ny >= 0 && ny < height)
            {
                if (world.getElevation(nx, ny) < 0.0f)
                {
                    nearWater = true;
                    int dist = std::abs(dx) + std::abs(dy);
                    waterDistance = std::min(waterDistance, dist);
                }
            }
        }
    }

    if (nearWater)
    {
        suitability += 20.0f * (1.0f - waterDistance / 10.0f);
    }
    else
    {
        suitability -= 10.0f;
    }

    // Temperature preference
    float temperature = climate.getTemperature(x, y);
    if (temperature > 5.0f && temperature < 25.0f)
    {
        suitability += 10.0f;
    }

    return std::max(0.0f, suitability);
}

bool CivilizationSystem::canPlaceCity(int x, int y, int minDistance)
{
    for (const auto &city : cities)
    {
        int dx = city->x - x;
        int dy = city->y - y;
        float distance = std::sqrt(dx * dx + dy * dy);
        if (distance < minDistance)
        {
            return false;
        }
    }
    return true;
}

void CivilizationSystem::placeInitialCities(const World &world, const ClimateSystem &climate, int numCities)
{
    std::cout << "Placing initial cities..." << std::endl;

    // Find best sites for cities
    std::vector<std::tuple<float, int, int>> potentialSites;

    for (int y = 10; y < height - 10; y += 2)
    {
        for (int x = 10; x < width - 10; x += 2)
        {
            float suitability = calculateSiteSuitability(world, climate, x, y);
            if (suitability > 0)
            {
                potentialSites.push_back({suitability, x, y});
            }
        }
    }

    // Sort by suitability
    std::sort(potentialSites.begin(), potentialSites.end(),
              [](const auto &a, const auto &b)
              { return std::get<0>(a) > std::get<0>(b); });

    // Place cities at best sites with minimum distance
    int citiesPlaced = 0;
    for (const auto &[suitability, x, y] : potentialSites)
    {
        if (canPlaceCity(x, y))
        {
            auto city = std::make_unique<City>(x, y, generateCityName(), currentYear);

            // Capital city gets bonus
            if (citiesPlaced == 0)
            {
                city->population = 500;
                city->resources = 200.0f;
                city->name = "Capital " + city->name;
            }

            cities.push_back(std::move(city));
            expandTerritory(citiesPlaced, world);

            std::cout << "  Founded " << cities.back()->name
                      << " at (" << x << ", " << y << ")"
                      << " with suitability " << suitability << std::endl;

            citiesPlaced++;
            if (citiesPlaced >= numCities)
                break;
        }
    }
}

void CivilizationSystem::connectCities()
{
    std::cout << "Building road network..." << std::endl;

    // Connect each city to its nearest neighbors
    for (size_t i = 0; i < cities.size(); i++)
    {
        std::vector<std::pair<float, size_t>> distances;

        // Calculate distances to all other cities
        for (size_t j = 0; j < cities.size(); j++)
        {
            if (i != j)
            {
                float dx = cities[i]->x - cities[j]->x;
                float dy = cities[i]->y - cities[j]->y;
                float dist = std::sqrt(dx * dx + dy * dy);
                distances.push_back({dist, j});
            }
        }

        // Sort by distance
        std::sort(distances.begin(), distances.end());

        // Connect to 2-3 nearest cities
        int connections = std::min(3, (int)distances.size());
        for (int k = 0; k < connections; k++)
        {
            size_t j = distances[k].second;

            // Check if already connected
            bool alreadyConnected = false;
            for (int conn : cities[i]->connectedCities)
            {
                if (conn == j)
                {
                    alreadyConnected = true;
                    break;
                }
            }

            if (!alreadyConnected)
            {
                // Find path between cities
                auto path = findPath(cities[i]->x, cities[i]->y, cities[j]->x, cities[j]->y);

                if (!path.empty())
                {
                    auto road = std::make_unique<Road>(i, j);
                    road->path = path;
                    roads.push_back(std::move(road));

                    cities[i]->connectedCities.push_back(j);
                    cities[j]->connectedCities.push_back(i);
                }
            }
        }
    }

    std::cout << "Built " << roads.size() << " roads!" << std::endl;
}

std::vector<std::pair<int, int>> CivilizationSystem::findPath(int startX, int startY, int endX, int endY)
{
    // A* pathfinding
    struct Node
    {
        int x, y;
        float g, h, f;
        std::pair<int, int> parent;

        bool operator>(const Node &other) const
        {
            return f > other.f;
        }
    };

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
    std::set<std::pair<int, int>> closedSet;
    std::map<std::pair<int, int>, std::pair<int, int>> parentMap;
    std::map<std::pair<int, int>, float> gScore;

    // Heuristic function
    auto heuristic = [](int x1, int y1, int x2, int y2) -> float
    {
        return std::sqrt((float)((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)));
    };

    // Start node
    Node start{startX, startY, 0, heuristic(startX, startY, endX, endY), 0, {-1, -1}};
    start.f = start.g + start.h;
    openSet.push(start);
    gScore[{startX, startY}] = 0;

    while (!openSet.empty())
    {
        Node current = openSet.top();
        openSet.pop();

        // Goal reached
        if (current.x == endX && current.y == endY)
        {
            // Reconstruct path
            std::vector<std::pair<int, int>> path;
            std::pair<int, int> pos = {current.x, current.y};

            while (pos.first != -1)
            {
                path.push_back(pos);
                if (parentMap.find(pos) != parentMap.end())
                {
                    pos = parentMap[pos];
                }
                else
                {
                    break;
                }
            }

            std::reverse(path.begin(), path.end());
            return path;
        }

        closedSet.insert({current.x, current.y});

        // Check neighbors
        for (int dy = -1; dy <= 1; dy++)
        {
            for (int dx = -1; dx <= 1; dx++)
            {
                if (dx == 0 && dy == 0)
                    continue;

                int nx = current.x + dx;
                int ny = current.y + dy;

                if (nx < 0 || nx >= width || ny < 0 || ny >= height)
                    continue;
                if (closedSet.find({nx, ny}) != closedSet.end())
                    continue;
                if (movementCost[ny][nx] > 100.0f)
                    continue; // Impassable

                float tentativeG = current.g + movementCost[ny][nx] *
                                                   ((dx != 0 && dy != 0) ? 1.414f : 1.0f);

                if (gScore.find({nx, ny}) == gScore.end() || tentativeG < gScore[{nx, ny}])
                {
                    gScore[{nx, ny}] = tentativeG;
                    parentMap[{nx, ny}] = {current.x, current.y};

                    Node neighbor{nx, ny, tentativeG, heuristic(nx, ny, endX, endY), 0, {current.x, current.y}};
                    neighbor.f = neighbor.g + neighbor.h;
                    openSet.push(neighbor);
                }
            }
        }
    }

    return {}; // No path found
}

void CivilizationSystem::simulate(const World &world, const ClimateSystem &climate)
{
    currentYear++;

    // Grow cities
    for (size_t i = 0; i < cities.size(); i++)
    {
        growCity(*cities[i], world, climate);
    }

    // Expand territories
    for (size_t i = 0; i < cities.size(); i++)
    {
        expandTerritory(i, world);
    }

    // Update development
    updateDevelopment();

    // Occasionally found new cities
    if (currentYear % 50 == 0 && cities.size() < 20)
    {
        // Find best unoccupied site
        float bestSuitability = 0;
        int bestX = -1, bestY = -1;

        for (int y = 10; y < height - 10; y += 5)
        {
            for (int x = 10; x < width - 10; x += 5)
            {
                if (territoryMap[y][x] == -1 && canPlaceCity(x, y, 15))
                {
                    float suit = calculateSiteSuitability(world, climate, x, y);
                    if (suit > bestSuitability)
                    {
                        bestSuitability = suit;
                        bestX = x;
                        bestY = y;
                    }
                }
            }
        }

        if (bestX != -1 && bestSuitability > 20)
        {
            auto city = std::make_unique<City>(bestX, bestY, generateCityName(), currentYear);
            cities.push_back(std::move(city));
            expandTerritory(cities.size() - 1, world);
            connectCities(); // Rebuild road network

            std::cout << "Year " << currentYear << ": Founded new city "
                      << cities.back()->name << std::endl;
        }
    }
}

void CivilizationSystem::growCity(City &city, const World &world, const ClimateSystem &climate)
{
    // Growth factors
    float growthModifier = 1.0f;

    // Biome affects growth
    BiomeType biome = climate.getBiome(city.x, city.y);
    switch (biome)
    {
    case BiomeType::TEMPERATE_GRASSLAND:
    case BiomeType::TEMPERATE_FOREST:
        growthModifier *= 1.2f;
        break;
    case BiomeType::DESERT:
    case BiomeType::TUNDRA:
    case BiomeType::ICE:
        growthModifier *= 0.7f;
        break;
    default:
        break;
    }

    // Trade connections boost growth
    growthModifier *= 1.0f + (city.connectedCities.size() * 0.1f);

    // Apply growth
    city.population = (int)(city.population * city.growthRate * growthModifier);
    city.resources += city.population * 0.01f;

    // Larger cities grow slower
    if (city.population > 1000)
    {
        city.growthRate = 1.015f;
    }
    if (city.population > 5000)
    {
        city.growthRate = 1.01f;
    }
    if (city.population > 10000)
    {
        city.growthRate = 1.005f;
    }
}

void CivilizationSystem::expandTerritory(int cityIndex, const World &world)
{
    if (cityIndex >= cities.size())
        return;

    const City &city = *cities[cityIndex];
    int radius = 5 + (city.population / 1000);

    for (int dy = -radius; dy <= radius; dy++)
    {
        for (int dx = -radius; dx <= radius; dx++)
        {
            int x = city.x + dx;
            int y = city.y + dy;

            if (x >= 0 && x < width && y >= 0 && y < height)
            {
                float distance = std::sqrt(dx * dx + dy * dy);
                if (distance <= radius && world.getElevation(x, y) > 0)
                {
                    if (territoryMap[y][x] == -1)
                    {
                        territoryMap[y][x] = cityIndex;
                    }
                }
            }
        }
    }
}

void CivilizationSystem::updateDevelopment()
{
    // Decay development
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            developmentMap[y][x] *= 0.99f;
        }
    }

    // Cities increase local development
    for (const auto &city : cities)
    {
        float devRadius = 3.0f + (city->population / 2000.0f);
        float devStrength = std::min(1.0f, city->population / 10000.0f);

        for (int dy = -devRadius; dy <= devRadius; dy++)
        {
            for (int dx = -devRadius; dx <= devRadius; dx++)
            {
                int x = city->x + dx;
                int y = city->y + dy;

                if (x >= 0 && x < width && y >= 0 && y < height)
                {
                    float distance = std::sqrt(dx * dx + dy * dy);
                    if (distance <= devRadius)
                    {
                        float influence = devStrength * (1.0f - distance / devRadius);
                        developmentMap[y][x] = std::min(1.0f, developmentMap[y][x] + influence * 0.1f);
                    }
                }
            }
        }
    }

    // Roads increase development along their path
    for (const auto &road : roads)
    {
        for (const auto &[x, y] : road->path)
        {
            if (x >= 0 && x < width && y >= 0 && y < height)
            {
                developmentMap[y][x] = std::min(1.0f, developmentMap[y][x] + 0.05f);
            }
        }
    }
}

void CivilizationSystem::render(sf::RenderWindow &window, int tileSize)
{
    // Render roads as dotted lines
    for (const auto &road : roads)
    {
        // Draw road segments with gaps for dotted effect
        for (size_t i = 0; i < road->path.size() - 1; i++)
        {
            // Only draw every other segment for dotted line effect
            if (i % 3 < 2)
            {
                sf::Vertex line[2];
                line[0].position = sf::Vector2f(
                    road->path[i].first * tileSize + tileSize / 2,
                    road->path[i].second * tileSize + tileSize / 2);
                line[0].color = sf::Color(101, 67, 33); // Dark brown
                line[1].position = sf::Vector2f(
                    road->path[i + 1].first * tileSize + tileSize / 2,
                    road->path[i + 1].second * tileSize + tileSize / 2);
                line[1].color = sf::Color(101, 67, 33);
                window.draw(line, 2, sf::PrimitiveType::Lines);
            }
        }
    }

    // Render cities as buildings
    for (const auto &city : cities)
    {
        float x = city->x * tileSize;
        float y = city->y * tileSize;

        // City size scales with population - DOUBLED base scale
        float scale = 1.6f + std::min(2.4f, (float)(std::log10(city->population + 1) * 0.6f));

        // Draw city based on size
        if (city->population < 1000)
        {
            // Small village - simple hut
            // Roof (triangle)
            sf::ConvexShape roof;
            roof.setPointCount(3);
            roof.setPoint(0, sf::Vector2f(x + tileSize / 2, y - 4 * scale));
            roof.setPoint(1, sf::Vector2f(x - 4 * scale, y + tileSize / 2));
            roof.setPoint(2, sf::Vector2f(x + tileSize + 4 * scale, y + tileSize / 2));
            roof.setFillColor(sf::Color(139, 69, 19)); // Brown roof
            window.draw(roof);

            // Base (rectangle)
            sf::RectangleShape base(sf::Vector2f(tileSize * scale * 2, tileSize * 1.2f * scale));
            base.setPosition(sf::Vector2f(x + tileSize * (1 - scale) / 2 - tileSize * scale / 2, y + tileSize * 0.4f));
            base.setFillColor(sf::Color(222, 184, 135)); // Tan walls
            window.draw(base);

            // Door
            sf::RectangleShape door(sf::Vector2f(tileSize * 0.4f * scale, tileSize * 0.8f * scale));
            door.setPosition(sf::Vector2f(x + tileSize * 0.3f, y + tileSize * 0.6f));
            door.setFillColor(sf::Color(101, 67, 33)); // Dark brown door
            window.draw(door);
        }
        else if (city->population < 5000)
        {
            // Medium town - multiple buildings
            for (int i = 0; i < 3; i++)
            {
                float offsetX = (i - 1) * tileSize * 0.8f * scale;
                float offsetY = (i == 1) ? -tileSize * 0.4f * scale : 0;

                // Building
                sf::RectangleShape building(sf::Vector2f(tileSize * 1.0f * scale, tileSize * 1.4f * scale));
                building.setPosition(sf::Vector2f(x + offsetX + tileSize * 0.0f, y + offsetY + tileSize * 0.1f));
                building.setFillColor(sf::Color(205, 133, 63)); // Peru color
                window.draw(building);

                // Roof
                sf::ConvexShape smallRoof;
                smallRoof.setPointCount(3);
                smallRoof.setPoint(0, sf::Vector2f(x + offsetX + tileSize / 2, y + offsetY - tileSize * 0.2f * scale));
                smallRoof.setPoint(1, sf::Vector2f(x + offsetX - tileSize * 0.1f * scale, y + offsetY + tileSize * 0.2f));
                smallRoof.setPoint(2, sf::Vector2f(x + offsetX + tileSize * 1.1f * scale, y + offsetY + tileSize * 0.2f));
                smallRoof.setFillColor(sf::Color(139, 69, 19));
                window.draw(smallRoof);
            }
        }
        else
        {
            // Large city - castle/tower
            // Tower base
            sf::RectangleShape tower(sf::Vector2f(tileSize * 1.6f * scale, tileSize * 2.0f * scale));
            tower.setPosition(sf::Vector2f(x - tileSize * 0.3f * scale, y - tileSize * 0.5f * scale));
            tower.setFillColor(sf::Color(105, 105, 105)); // Dim gray stone
            window.draw(tower);

            // Tower top
            sf::RectangleShape towerTop(sf::Vector2f(tileSize * 2.0f * scale, tileSize * 0.6f * scale));
            towerTop.setPosition(sf::Vector2f(x - tileSize * 0.5f * scale, y - tileSize * 1.1f * scale));
            towerTop.setFillColor(sf::Color(105, 105, 105));
            window.draw(towerTop);

            // Battlements
            for (int i = 0; i < 4; i++)
            {
                sf::RectangleShape battlement(sf::Vector2f(tileSize * 0.3f * scale, tileSize * 0.4f * scale));
                battlement.setPosition(sf::Vector2f(x - tileSize * 0.4f * scale + i * tileSize * 0.5f * scale, y - tileSize * 1.5f * scale));
                battlement.setFillColor(sf::Color(105, 105, 105));
                window.draw(battlement);
            }

            // Flag (for capital or large cities)
            if (city->population > 10000)
            {
                // Flag pole
                sf::RectangleShape pole(sf::Vector2f(4, tileSize * 1.2f * scale));
                pole.setPosition(sf::Vector2f(x + tileSize * 0.9f * scale, y - tileSize * 2.0f * scale));
                pole.setFillColor(sf::Color(101, 67, 33));
                window.draw(pole);

                // Flag
                sf::ConvexShape flag;
                flag.setPointCount(3);
                flag.setPoint(0, sf::Vector2f(x + tileSize * 0.9f * scale + 4, y - tileSize * 2.0f * scale));
                flag.setPoint(1, sf::Vector2f(x + tileSize * 0.9f * scale + tileSize * 0.6f, y - tileSize * 1.6f * scale));
                flag.setPoint(2, sf::Vector2f(x + tileSize * 0.9f * scale + 4, y - tileSize * 1.2f * scale));
                flag.setFillColor(sf::Color(220, 20, 60)); // Crimson
                window.draw(flag);
            }
        }

        // City name label (optional - comment out if too cluttered)
        // Would need font support in SFML 3.0
    }
}

void CivilizationSystem::renderTerritory(sf::RenderWindow &window, int tileSize)
{
    // Color palette for different civilizations
    std::vector<sf::Color> territoryColors = {
        sf::Color(255, 0, 0, 100),   // Red
        sf::Color(0, 0, 255, 100),   // Blue
        sf::Color(0, 255, 0, 100),   // Green
        sf::Color(255, 255, 0, 100), // Yellow
        sf::Color(255, 0, 255, 100), // Magenta
        sf::Color(0, 255, 255, 100), // Cyan
        sf::Color(255, 128, 0, 100), // Orange
        sf::Color(128, 0, 255, 100), // Purple
    };

    sf::VertexArray vertices(sf::PrimitiveType::Triangles, width * height * 6);
    int index = 0;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int territory = territoryMap[y][x];
            if (territory >= 0)
            {
                sf::Color color = territoryColors[territory % territoryColors.size()];

                float left = x * tileSize;
                float top = y * tileSize;
                float right = left + tileSize;
                float bottom = top + tileSize;

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

                index += 6;
            }
            else
            {
                index += 6;
            }
        }
    }

    vertices.resize(index);
    window.draw(vertices);
}

void CivilizationSystem::renderDevelopment(sf::RenderWindow &window, int tileSize)
{
    sf::VertexArray vertices(sf::PrimitiveType::Triangles, width * height * 6);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int index = (y * width + x) * 6;

            float development = developmentMap[y][x];
            if (development > 0.01f)
            {
                // Yellow to red gradient for development
                int red = 255;
                int green = 255 * (1.0f - development);
                sf::Color color(red, green, 0, 150);

                float left = x * tileSize;
                float top = y * tileSize;
                float right = left + tileSize;
                float bottom = top + tileSize;

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
    }

    window.draw(vertices);
}

int CivilizationSystem::getTotalPopulation() const
{
    int total = 0;
    for (const auto &city : cities)
    {
        total += city->population;
    }
    return total;
}