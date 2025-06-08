#include <SFML/Graphics.hpp>
#include <optional>

int main()
{
    // SFML 3.0 uses Vector2u for VideoMode constructor
    sf::RenderWindow window(sf::VideoMode({800u, 600u}), "Genesis Engine");

    while (window.isOpen())
    {
        // SFML 3.0 pollEvent() returns std::optional<Event>
        while (std::optional event = window.pollEvent())
        {
            // SFML 3.0 uses is<T>() to check event types
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        window.clear(sf::Color::Black);
        // Drawing will happen here later
        window.display();
    }

    return 0;
}