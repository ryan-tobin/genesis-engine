#include <SFML/Graphics.hpp>
#include <optional>
#include <iostream>
#include <chrono>
#include <algorithm>
#include "World.h"

int main()
{
    // Window settings
    const unsigned int windowWidth = 1200;
    const unsigned int windowHeight = 800;

    // World settings - Larger world with smaller tiles for more detail
    const int worldWidth = 300;
    const int worldHeight = 200;
    const int tileSize = 12;

    // Create window
    sf::RenderWindow window(sf::VideoMode({windowWidth, windowHeight}), "Genesis Engine - Phase 1: Terrain Generation");
    window.setFramerateLimit(60);

    // Create world with a seed based on current time
    auto now = std::chrono::system_clock::now();
    auto seed = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    World world(worldWidth, worldHeight, tileSize, seed);

    // Generate initial world
    std::cout << "Generating world with seed: " << seed << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  WASD/Arrow Keys - Move camera" << std::endl;
    std::cout << "  Mouse Wheel - Zoom in/out" << std::endl;
    std::cout << "  R - Regenerate world with single island" << std::endl;
    std::cout << "  T - Generate archipelago (multiple islands)" << std::endl;
    std::cout << "  Space - Reset camera view" << std::endl;
    world.generateNoiseMap();
    world.assignTerrainTypes();
    std::cout << "World generation complete!" << std::endl;

    // Create view for camera control
    sf::View view;
    view.setSize(sf::Vector2f(windowWidth, windowHeight));
    view.setCenter(sf::Vector2f(worldWidth * tileSize / 2.0f, worldHeight * tileSize / 2.0f));
    window.setView(view);

    // Camera movement speed
    const float cameraSpeed = 300.0f; // pixels per second
    sf::Clock deltaClock;

    // Zoom limits
    const float minZoom = 0.1f;
    const float maxZoom = 5.0f;
    float currentZoom = 1.0f;

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
                // Regenerate world with new seed (single island)
                if (keyEvent->code == sf::Keyboard::Key::R)
                {
                    seed = std::chrono::duration_cast<std::chrono::seconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();

                    world = World(worldWidth, worldHeight, tileSize, seed);
                    world.setIslandMode(World::IslandMode::SINGLE);
                    std::cout << "Regenerating world with seed: " << seed << " (Single Island)" << std::endl;
                    world.generateNoiseMap();
                    world.assignTerrainTypes();
                    std::cout << "World regeneration complete!" << std::endl;
                }
                // Generate archipelago
                else if (keyEvent->code == sf::Keyboard::Key::T)
                {
                    seed = std::chrono::duration_cast<std::chrono::seconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();

                    world = World(worldWidth, worldHeight, tileSize, seed);
                    world.setIslandMode(World::IslandMode::ARCHIPELAGO);
                    std::cout << "Regenerating world with seed: " << seed << " (Archipelago)" << std::endl;
                    world.generateNoiseMap();
                    world.assignTerrainTypes();
                    std::cout << "World regeneration complete!" << std::endl;
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

        // Camera movement with WASD or arrow keys
        sf::Vector2f movement(0.0f, 0.0f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            movement.y -= cameraSpeed * deltaTime * currentZoom;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            movement.y += cameraSpeed * deltaTime * currentZoom;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            movement.x -= cameraSpeed * deltaTime * currentZoom;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            movement.x += cameraSpeed * deltaTime * currentZoom;

        view.move(movement);

        // Apply camera bounds
        sf::Vector2f viewCenter = view.getCenter();
        sf::Vector2f viewSize = view.getSize();

        // Calculate bounds
        float minX = viewSize.x / 2.0f;
        float minY = viewSize.y / 2.0f;
        float maxX = worldWidth * tileSize - viewSize.x / 2.0f;
        float maxY = worldHeight * tileSize - viewSize.y / 2.0f;

        // Ensure bounds are valid even when zoomed out far
        if (minX > maxX)
        {
            minX = maxX = (worldWidth * tileSize) / 2.0f;
        }
        if (minY > maxY)
        {
            minY = maxY = (worldHeight * tileSize) / 2.0f;
        }

        // Clamp view to world bounds
        viewCenter.x = std::max(minX, std::min(maxX, viewCenter.x));
        viewCenter.y = std::max(minY, std::min(maxY, viewCenter.y));
        view.setCenter(viewCenter);

        window.setView(view);

        // Render
        window.clear(sf::Color::Black);
        world.render(window);

        // Draw UI instructions
        window.setView(window.getDefaultView());

        // For now, we'll skip the text rendering since SFML 3.0 requires a font at construction
        // You can add this back later with a proper font file
        /*
        sf::Font font;
        if (font.openFromFile("arial.ttf"))
        {
            sf::Text instructions(font, "Controls: WASD/Arrows - Move | Mouse Wheel - Zoom | R - Regenerate | Space - Reset View", 14);
            instructions.setFillColor(sf::Color::White);
            instructions.setPosition(sf::Vector2f(10, 10));
            window.draw(instructions);
        }
        */

        window.setView(view);

        window.display();
    }

    return 0;
}