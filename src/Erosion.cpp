#include "Erosion.h"
#include "World.h"
#include <cmath>
#include <algorithm>
#include <iostream>

ErosionSimulator::ErosionSimulator(unsigned int seed) : rng(seed), uniformDist(0.0f, 1.0f) {}

void ErosionSimulator::erode(World &world, int numDroplets)
{
    if (numDroplets < 0)
    {
        numDroplets = params.numDroplets;
    }

    std::cout << "Starting erosion simulation with " << numDroplets << " droplets..." << std::endl;

    int progressInterval = numDroplets / 10;

    for (int i = 0; i < numDroplets; i++)
    {
        float startX = uniformDist(rng) * (world.getWidth() - 1);
        float startY = uniformDist(rng) * (world.getHeight() - 1);

        if (world.getElevation((int)startX, (int)startY) < -0.1f)
        {
            continue;
        }

        Droplet droplet(startX, startY);
        droplet.water = params.startWater;
        droplet.velocity = params.startVelocity;

        simulateDroplet(world, droplet);

        if (i % progressInterval == 0)
        {
            std::cout << "Erosion progress: " << (i * 100 / numDroplets) << "%" << std::endl;
        }
    }

    std::cout << "Erosion simulation complete." << std::endl;

    world.assignTerrainTypes();
}

void ErosionSimulator::simulateDroplet(World &world, Droplet &droplet)
{
    for (int lifetime = 0; lifetime < params.maxLifetime; lifetime++)
    {
        int nodeX = (int)droplet.x;
        int nodeY = (int)droplet.y;

        if (nodeX < 0 || nodeX >= world.getWidth() - 1 || nodeY < 0 || nodeY >= world.getHeight() - 1)
        {
            break;
        }
        float height, gradX, gradY;
        getHeightAndGradient(world, droplet.x, droplet.y, height, gradX, gradY);

        droplet.dx = droplet.dx * params.inertia - gradX * (1 - params.inertia);
        droplet.dy = droplet.dy * params.inertia - gradY * (1 - params.inertia);

        float len = std::sqrt(droplet.dx * droplet.dx + droplet.dy * droplet.dy);
        if (len != 0)
        {
            droplet.dx /= len;
            droplet.dy /= len;
        }

        float oldX = droplet.x;
        float oldY = droplet.y;
        droplet.x += droplet.dx;
        droplet.y += droplet.dy;

        if ((droplet.dx == 0 && droplet.dy == 0) || droplet.x < 0 || droplet.x >= world.getWidth() - 1 || droplet.y < 0 || droplet.y >= world.getHeight() - 1)
        {
            break;
        }

        float newHeight, newGradX, newGradY;
        getHeightAndGradient(world, droplet.x, droplet.y, newHeight, newGradX, newGradY);

        float deltaHeight = newHeight - height;

        float slope = std::max(-deltaHeight, params.minSlope);
        float capacity = slope * droplet.velocity * droplet.water * params.capacity;

        if (droplet.sediment > capacity || deltaHeight > 0)
        {
            float amountToDeposit = (deltaHeight > 0) ? std::min(deltaHeight, droplet.sediment) : (droplet.sediment - capacity) * params.deposition;

            droplet.sediment -= amountToDeposit;

            float cellOffsetX = oldX - nodeX;
            float cellOffsetY = oldY - nodeY;

            world.modifyElevation(nodeX, nodeY, amountToDeposit * (1 - cellOffsetX) * (1 - cellOffsetY));
            world.modifyElevation(nodeX + 1, nodeY, amountToDeposit * cellOffsetX * (1 - cellOffsetY));
            world.modifyElevation(nodeX, nodeY + 1, amountToDeposit * (1 - cellOffsetX) * cellOffsetY);
            world.modifyElevation(nodeX + 1, nodeY + 1, amountToDeposit * cellOffsetX * cellOffsetY);
        }
        else
        {
            float amountToErode = std::min((capacity - droplet.sediment) * params.erosion, -deltaHeight);

            for (int brushY = -1; brushY <= 1; brushY++)
            {
                for (int brushX = -1; brushX <= 1; brushX++)
                {
                    int erodeX = nodeX + brushX;
                    int erodeY = nodeY + brushY;

                    if (erodeX >= 0 && erodeX < world.getWidth() && erodeY >= 0 && erodeY < world.getHeight())
                    {
                        float distance = std::sqrt(brushX * brushX + brushY * brushY);
                        float weight = std::max(0.0f, 1.0f - distance);
                        float weightedErosion = amountToErode * weight * 0.25f;

                        world.modifyElevation(erodeX, erodeY, -weightedErosion);
                        droplet.sediment += weightedErosion;
                    }
                }
            }
        }

        droplet.velocity = std::sqrt(std::max(0.0f, droplet.velocity * droplet.velocity + deltaHeight * params.gravity));

        droplet.water *= (1 - params.evaporation);

        if (droplet.water < 0.001f)
        {
            break;
        }
    }
}

void ErosionSimulator::getHeightAndGradient(World &world, float x, float y, float &height, float &gradX, float &gradY)
{
    int coordX = (int)x;
    int coordY = (int)y;

    float u = x - coordX;
    float v = y - coordY;

    float heightNW = world.getElevation(coordX, coordY);
    float heightNE = world.getElevation(coordX + 1, coordY);
    float heightSW = world.getElevation(coordX, coordY + 1);
    float heightSE = world.getElevation(coordX + 1, coordY + 1);

    gradX = (heightNE - heightNW) * (1 - v) + (heightSE - heightSW) * v;
    gradY = (heightSW - heightNW) * (1 - u) + (heightSE - heightNE) * u;

    height = bilinearInterpolate(heightNW, heightNE, heightSW, heightSE, u, v);
}

float ErosionSimulator::bilinearInterpolate(float v00, float v10, float v01, float v11, float fx, float fy)
{
    return v00 * (1 - fx) * (1 - fy) +
           v10 * fx * (1 - fy) +
           v01 * (1 - fx) * fy +
           v11 * fx * fy;
}