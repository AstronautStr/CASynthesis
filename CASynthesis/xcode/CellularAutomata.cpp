//
//  CellularAutomata.cpp
//  CASynthesis
//
//  Created by Ilya Solovyov on 01.02.16.
//
//

#include "CellularAutomata.hpp"

float randFreq(float lowest, float highest)
{
    return lowest + highest * ((float)rand() / RAND_MAX);
}

float randFreqCentered(float center, float delta)
{
    return center + delta * (1.0 - ((float)rand() / RAND_MAX) * 2);
}