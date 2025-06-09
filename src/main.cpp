#include <SFML/Graphics.hpp>
#include <iostream>
#include <chrono>
#include <optional>
#include <algorithm>

#include "World.h"
#include "Erosion.h"
#include "Climate.h"
#include "Civilization.h"

int main()
{
    // Window settings
    const unsigned int windowWidth = 1200;
    const unsigned int windowHeight = 800;

    // World settings
    const int worldWidth = 300;
    const int worldHeight = 200;
    const int tileSize = 4;

    // Create window
    sf::RenderWindow window(sf::VideoMode({windowWidth, windowHeight}), "Genesis Engine - Phase 2: Civilization");
    window.setFramerateLimit(60);

    // Create world with a time-based seed
    auto now = std::chrono::system_clock::now();
    auto seed = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    World world(worldWidth, worldHeight, tileSize, seed);

    // Print controls to the console
    std::cout << "Generating world with seed: " << seed << std::endl;
    std::cout << "\nControls:" << std::endl;
    std::cout << "  Movement:" << std::endl;
    std::cout << "    WASD/Arrow Keys - Move camera" << std::endl;
    std::cout << "    Mouse Wheel     - Zoom in/out" << std::endl;
    std::cout << "    Space           - Reset camera view" << std::endl;
    std::cout << "\n  World Generation:" << std::endl;
    std::cout << "    R - Regenerate world (single island)" << std::endl;
    std::cout << "    T - Generate archipelago (multiple islands)" << std::endl;
    std::cout << "    E - Apply erosion simulation" << std::endl;
    std::cout << "    C - Generate climate and biomes" << std::endl;
    std::cout << "    V - Initialize civilization" << std::endl;
    std::cout << "    N - Next turn (simulate civilization)" << std::endl;
    std::cout << "\n  View Modes:" << std::endl;
    std::cout << "    1 - Terrain view" << std::endl;
    std::cout << "    2 - Heightmap view" << std::endl;
    std::cout << "    3 - Biome view" << std::endl;
    std::cout << "    4 - Temperature view" << std::endl;
    std::cout << "    5 - Moisture view" << std::endl;
    std::cout << "    6 - Civilization view" << std::endl;
    std::cout << "    7 - Territory view" << std::endl;
    std::cout << "    8 - Development view" << std::endl;
    std::cout << "\nRecommended sequence: R -> E -> C -> V -> N" << std::endl;

    // Generate initial world
    world.generateNoiseMap();
    world.assignTerrainTypes();
    std::cout << "World generation complete!" << std::endl;

    // Create simulation systems and state flags
    ErosionSimulator erosion(seed);
    ClimateSystem climate(worldWidth, worldHeight);
    CivilizationSystem civilization(worldWidth, worldHeight);
    bool climateGenerated = false;
    bool civilizationActive = false;

    // Create view for camera control
    sf::View view;
    view.setSize(sf::Vector2f(windowWidth, windowHeight));
    view.setCenter(sf::Vector2f(worldWidth * tileSize / 2.0f, worldHeight * tileSize / 2.0f));
    window.setView(view);

    // Camera settings
    const float cameraSpeed = 300.0f;
    const float minZoom = 0.1f;
    const float maxZoom = 5.0f;
    float currentZoom = 1.0f;
    sf::Clock deltaClock;

    // Visualization mode
    enum class ViewMode
    {
        TERRAIN,
        HEIGHTMAP,
        BIOMES,
        TEMPERATURE,
        MOISTURE,
        CIVILIZATION,
        TERRITORY,
        DEVELOPMENT
    };
    ViewMode viewMode = ViewMode::TERRAIN;

    // Main loop
    while (window.isOpen())
    {
        float deltaTime = deltaClock.restart().asSeconds();

        // Handle events
        while (std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (auto *keyEvent = event->getIf<sf::Event::KeyPressed>())
            {
                // Regenerate world (single island)
                if (keyEvent->code == sf::Keyboard::Key::R || keyEvent->code == sf::Keyboard::Key::T)
                {
                    seed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                    world = World(worldWidth, worldHeight, tileSize, seed);

                    if (keyEvent->code == sf::Keyboard::Key::R)
                    {
                        world.setIslandMode(World::IslandMode::SINGLE);
                        std::cout << "Regenerating world with seed: " << seed << " (Single Island)" << std::endl;
                    }
                    else
                    {
                        world.setIslandMode(World::IslandMode::ARCHIPELAGO);
                        std::cout << "Regenerating world with seed: " << seed << " (Archipelago)" << std::endl;
                    }

                    world.generateNoiseMap();
                    world.assignTerrainTypes();

                    // Reset dependent states
                    climateGenerated = false;
                    civilizationActive = false;
                    viewMode = ViewMode::TERRAIN;
                    std::cout << "World regeneration complete! Climate and civilization have been reset." << std::endl;
                }
                // Apply erosion
                else if (keyEvent->code == sf::Keyboard::Key::E)
                {
                    std::cout << "Applying hydraulic erosion..." << std::endl;
                    erosion.getParameters().erosion = 0.5f;
                    erosion.getParameters().capacity = 8.0f;
                    erosion.getParameters().maxLifetime = 50;
                    erosion.erode(world, 200000);
                    std::cout << "Erosion complete! Rivers and valleys carved." << std::endl;
                }
                // Generate climate
                else if (keyEvent->code == sf::Keyboard::Key::C)
                {
                    std::cout << "Generating climate and biomes..." << std::endl;
                    climate.generateClimate(world);
                    climateGenerated = true;
                    viewMode = ViewMode::BIOMES;
                    std::cout << "Climate generation complete! Switched to biome view." << std::endl;
                }
                // Initialize civilization
                else if (keyEvent->code == sf::Keyboard::Key::V)
                {
                    if (climateGenerated)
                    {
                        std::cout << "Initializing civilization..." << std::endl;
                        civilization.initialize(world, climate);
                        civilizationActive = true;
                        viewMode = ViewMode::CIVILIZATION;
                        std::cout << "Civilization started with " << civilization.getCityCount() << " cities!" << std::endl;
                    }
                    else
                    {
                        std::cout << "Please generate climate first (press C)" << std::endl;
                    }
                }
                // Simulate civilization turn
                else if (keyEvent->code == sf::Keyboard::Key::N)
                {
                    if (civilizationActive)
                    {
                        civilization.simulate(world, climate);
                        std::cout << "Year " << civilization.getYear()
                                  << " - Population: " << civilization.getTotalPopulation()
                                  << " in " << civilization.getCityCount() << " cities" << std::endl;
                    }
                    else
                    {
                        std::cout << "Initialize civilization first (press V)" << std::endl;
                    }
                }
                // View mode switches
                else if (keyEvent->code == sf::Keyboard::Key::Num1)
                {
                    viewMode = ViewMode::TERRAIN;
                    std::cout << "Switched to terrain view" << std::endl;
                }
                else if (keyEvent->code == sf::Keyboard::Key::Num2)
                {
                    viewMode = ViewMode::HEIGHTMAP;
                    std::cout << "Switched to heightmap view" << std::endl;
                }
                else if (keyEvent->code == sf::Keyboard::Key::Num3)
                {
                    viewMode = ViewMode::BIOMES;
                    std::cout << "Switched to biome view" << std::endl;
                }
                else if (keyEvent->code == sf::Keyboard::Key::Num4)
                {
                    viewMode = ViewMode::TEMPERATURE;
                    std::cout << "Switched to temperature view" << std::endl;
                }
                else if (keyEvent->code == sf::Keyboard::Key::Num5)
                {
                    viewMode = ViewMode::MOISTURE;
                    std::cout << "Switched to moisture view" << std::endl;
                }
                else if (keyEvent->code == sf::Keyboard::Key::Num6)
                {
                    viewMode = ViewMode::CIVILIZATION;
                    std::cout << "Switched to civilization view" << std::endl;
                }
                else if (keyEvent->code == sf::Keyboard::Key::Num7)
                {
                    viewMode = ViewMode::TERRITORY;
                    std::cout << "Switched to territory view" << std::endl;
                }
                else if (keyEvent->code == sf::Keyboard::Key::Num8)
                {
                    viewMode = ViewMode::DEVELOPMENT;
                    std::cout << "Switched to development view" << std::endl;
                }
                // Reset camera
                else if (keyEvent->code == sf::Keyboard::Key::Space)
                {
                    view.setCenter(sf::Vector2f(worldWidth * tileSize / 2.0f, worldHeight * tileSize / 2.0f));
                    view.setSize(sf::Vector2f(windowWidth, windowHeight));
                    currentZoom = 1.0f;
                }
            }
            else if (auto *scrollEvent = event->getIf<sf::Event::MouseWheelScrolled>())
            {
                // Zoom in/out with limits
                float zoomFactor = scrollEvent->delta > 0 ? 0.9f : 1.1f;
                float newZoom = currentZoom * zoomFactor;
                if (newZoom >= minZoom && newZoom <= maxZoom)
                {
                    view.zoom(zoomFactor);
                    currentZoom = newZoom;
                }
            }
        }

        // Camera movement
        sf::Vector2f movement(0.0f, 0.0f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
        {
            movement.y -= cameraSpeed * deltaTime * currentZoom;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
        {
            movement.y += cameraSpeed * deltaTime * currentZoom;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        {
            movement.x -= cameraSpeed * deltaTime * currentZoom;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        {
            movement.x += cameraSpeed * deltaTime * currentZoom;
        }
        view.move(movement);

        // Apply camera bounds
        sf::Vector2f viewCenter = view.getCenter();
        sf::Vector2f viewSize = view.getSize();
        float minX = viewSize.x / 2.0f;
        float minY = viewSize.y / 2.0f;
        float maxX = worldWidth * tileSize - viewSize.x / 2.0f;
        float maxY = worldHeight * tileSize - viewSize.y / 2.0f;
        if (minX > maxX)
        {
            minX = maxX = (worldWidth * tileSize) / 2.0f;
        }
        if (minY > maxY)
        {
            minY = maxY = (worldHeight * tileSize) / 2.0f;
        }
        viewCenter.x = std::max(minX, std::min(maxX, viewCenter.x));
        viewCenter.y = std::max(minY, std::min(maxY, viewCenter.y));
        view.setCenter(viewCenter);
        window.setView(view);

        // Render everything
        window.clear(sf::Color::Black);

        switch (viewMode)
        {
        case ViewMode::TERRAIN:
            world.render(window);
            break;
        case ViewMode::HEIGHTMAP:
            world.renderHeightmap(window);
            break;
        case ViewMode::BIOMES:
            if (climateGenerated)
                climate.render(window, tileSize);
            else
                world.render(window);
            break;
        case ViewMode::TEMPERATURE:
            if (climateGenerated)
                climate.renderTemperature(window, tileSize);
            else
                world.render(window);
            break;
        case ViewMode::MOISTURE:
            if (climateGenerated)
                climate.renderMoisture(window, tileSize);
            else
                world.render(window);
            break;
        case ViewMode::CIVILIZATION:
            if (climateGenerated)
                climate.render(window, tileSize);
            else
                world.render(window);
            if (civilizationActive)
            {
                civilization.render(window, tileSize);
            }
            break;
        case ViewMode::TERRITORY:
            world.render(window);
            if (civilizationActive)
            {
                civilization.renderTerritory(window, tileSize);
            }
            break;
        case ViewMode::DEVELOPMENT:
            world.render(window);
            if (civilizationActive)
            {
                civilization.renderDevelopment(window, tileSize);
                civilization.render(window, tileSize);
            }
            break;
        }

        window.display();
    }

    return 0;
}