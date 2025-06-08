#pragma once

#include <vector>
#include <random>

class World;

struct Droplet
{
    float x, y;
    float dx, dy;
    float velocity;
    float water;
    float sediment;

    Droplet(float x, float y) : x(x), y(y), dx(0), dy(0), velocity(1), water(1), sediment(0) {}
};

class ErosionSimulator
{
private:
    struct Parameters
    {
        int numDroplets = 100000;
        float inertia = 0.05f;
        float capacity = 4.0f;
        float deposition = 0.3f;
        float erosion = 0.3f;
        float evaporation = 0.01f;
        float gravity = 4.0f;
        float minSlope = 0.01f;
        int maxLifetime = 30;
        float startWater = 1.0f;
        float startVelocity = 1.0f;
    } params;

    std::mt19937 rng;
    std::uniform_real_distribution<float> uniformDist;

    void simulateDroplet(World &world, Droplet &droplet);
    void getHeightAndGradient(World &world, float x, float y, float &height, float &gradX, float &gradY);
    float bilinearInterpolate(float v00, float v10, float v01, float v11, float fx, float fy);

public:
    ErosionSimulator(unsigned int seed = 0);

    void erode(World &world, int numDroplets = -1);

    void setErosionStrength(float strength) { params.erosion = strength; }
    void setDepositionRate(float rate) { params.deposition = rate; }
    void setEvaporationRate(float rate) { params.evaporation = rate; }
    void setCapacityMultiplier(float mult) { params.capacity = mult; }

    Parameters &getParameters() { return params; }
};