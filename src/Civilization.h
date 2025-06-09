#pragma once

#include <vector>
#include <string>
#include <memory>
#include <SFML/Graphics.hpp>
#include "Climate.h"

class World;
class ClimateSystem;

struct City
{
    int x, y;
    std::string name;
    int population;
    int foundingYear;
    float resources;
    float growthRate;
    std::vector<int> connectedCities; // Indices of connected cities

    City(int x, int y, const std::string &name, int year)
        : x(x), y(y), name(name), population(100),
          foundingYear(year), resources(50.0f), growthRate(1.02f) {}
};

struct Road
{
    std::vector<std::pair<int, int>> path;
    int cityA, cityB;
    float usage;

    Road(int a, int b) : cityA(a), cityB(b), usage(0.0f) {}
};

class CivilizationSystem
{
private:
    int width;
    int height;
    int currentYear;

    std::vector<std::unique_ptr<City>> cities;
    std::vector<std::unique_ptr<Road>> roads;
    std::vector<std::vector<int>> territoryMap;     // -1 = unclaimed, else city index
    std::vector<std::vector<float>> developmentMap; // 0-1 development level

    // Pathfinding grid
    std::vector<std::vector<float>> movementCost;

    // City name generator
    std::vector<std::string> namePrefix = {
        "New", "Port", "Mount", "Lake", "North", "South", "East", "West",
        "Fort", "Saint", "Royal", "Grand", "Old", "Upper", "Lower"};

    std::vector<std::string> nameSuffix = {
        "haven", "burg", "ville", "ton", "ford", "bridge", "field",
        "wood", "hill", "vale", "shore", "cliff", "rapids", "falls",
        "meadow", "grove", "ridge", "crest", "view", "harbor"};

    // Helper functions
    std::string generateCityName();
    float calculateSiteSuitability(const World &world, const ClimateSystem &climate, int x, int y);
    bool canPlaceCity(int x, int y, int minDistance = 20);
    void growCity(City &city, const World &world, const ClimateSystem &climate);
    void expandTerritory(int cityIndex, const World &world);
    void updateDevelopment();

    // Pathfinding
    std::vector<std::pair<int, int>> findPath(int startX, int startY, int endX, int endY);
    void calculateMovementCosts(const World &world, const ClimateSystem &climate);

public:
    CivilizationSystem(int width, int height);

    void initialize(const World &world, const ClimateSystem &climate);
    void simulate(const World &world, const ClimateSystem &climate);
    void placeInitialCities(const World &world, const ClimateSystem &climate, int numCities = 5);
    void connectCities();

    void render(sf::RenderWindow &window, int tileSize);
    void renderTerritory(sf::RenderWindow &window, int tileSize);
    void renderDevelopment(sf::RenderWindow &window, int tileSize);

    // Getters
    int getYear() const { return currentYear; }
    int getCityCount() const { return cities.size(); }
    int getTotalPopulation() const;
    const std::vector<std::unique_ptr<City>> &getCities() const { return cities; }
};