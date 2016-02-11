//
//  CellularAutomata.cpp
//  CASynthesis
//
//  Created by Ilya Solovyov on 01.02.16.
//
//

#include "CellularAutomata.hpp"

#define FREQ_LOW
#define FREQ_HIGH

float randFreq(float lowest, float highest)
{
    return lowest + highest * ((float)rand() / RAND_MAX);
}

float randFreqCentered(float center, float delta)
{
    return center + delta * (1.0 - ((float)rand() / RAND_MAX) * 2);
}


void Cell::_callDelegate()
{
    if (_delegate != NULL)
        _delegate->cellStateChanged(this);
}

Cell::Cell(Grid* host, int x, int y, bool alive, CellDelegate* delegate)
{
    _host = host;
    _x = x;
    _y = y;
    
    osc = ci::audio::master()->makeNode(new ci::audio::GenSineNode);
    gain = ci::audio::master()->makeNode(new ci::audio::GainNode);
    osc->enable();
    osc >> gain >> ci::audio::master()->getOutput();
    //osc->setFreq(_freq);
    //gain->setValue(0.0);
    
    _freq = randFreq();
    _delegate = delegate;
    
    setAlive(alive);
}

int Cell::getX()
{
    return _x;
}

int Cell::getY()
{
    return _y;
}

bool Cell::isAlive()
{
    //return _isAlive;
    return _energy > 0;
}

void Cell::setAlive(bool alive)
{
    /*if (_isAlive != alive)
     _energy = alive ? 1.0 : 0.0;
     
     _isAlive = alive;*/
    setEnergy(alive ? _host->getLifePower() : 0.0);
}

float Cell::getFreq()
{
    return _freq;
}

void Cell::setEnergy(float energy)
{
    _energy = ci::math<float>::clamp(energy);
    if (gain != nullptr)
    {
        //gain->setValue(_energy / 64.0);
        gain->getParam()->appendRamp(_energy / 64.0, 0.1);
    }
    
    _callDelegate();
}

float Cell::getEnergy()
{
    return _energy;
}

void Cell::setDelegate(CellDelegate* delegate)
{
    _delegate = delegate;
}

void Cell::setFreq(float newFreq)
{
    _freq = newFreq;
    
    if (osc != nullptr)
    {
        osc->setFreq(_freq);
    }
}


int Grid::_cycledIndex(int index, int length)
{
    if (index < 0)
        return _cycledIndex(length + index, length);
    else if (index >= length)
        return _cycledIndex(index - length, length);
    else
        return index;
}

void Grid::_applyRuleRecursively(int i, int j)
{
    if (i < 0 || i >= _width || j < 0 || j >=_height)
        return;
    
    Cell* currentCell = _cellsGrid[i][j];
    
    //bool futureState = currentCell->isAlive();
    //float futureEnergy = 0;
    float broEnergy = 0;
    float energyDelta = 0;
    float futureFreq = currentCell->getFreq();
    
    unsigned int aliveBroCount = 0;
    
    if (currentCell != NULL)
    {
        const int radius = 1;
        //float centerFreq = 0;
        for (int nx = -radius; nx <= radius; ++nx)
        {
            for (int ny = -radius; ny <= radius; ++ny)
            {
                Cell* bro = _cellsGrid[_cycledIndex(i + nx, _width)][_cycledIndex(j + ny, _height)];
                if ((nx != 0 || ny != 0) && bro->isAlive())
                {
                    aliveBroCount++;
                    
                    
                    float diff = cinder::math<float>::max(bro->getFreq(), currentCell->getFreq()) / cinder::math<float>::min(bro->getFreq(), currentCell->getFreq());
                    
                    float dE = (1.0 - pow(fabs(20 * (diff - (int)diff)), 0.25));
                    broEnergy += dE / 2;
                    
                    //centerFreq += bro->getFreq();
                }
            }
        }
        
        if (currentCell->isAlive())
        {
            if (aliveBroCount == 2 || aliveBroCount == 3)
            {
                //futureState = true;
                //energyFlow += DEATH_POWER;
            }
            else
            {
                energyDelta -= getLifePower();
            }
        }
        else
        {
            if (aliveBroCount == 3)
            {
                //futureState = true;
                energyDelta += getLifePower();
                //centerFreq /= aliveBroCount;
                futureFreq = randFreq();//randFreqCentered(centerFreq, centerFreq * 0.7);
            }
        }
    }
    
    if (j + 1 < _height)
        _applyRuleRecursively(i, j + 1);
    else
    {
        j = 0;
        
        if (i + 1 < _width)
            _applyRuleRecursively(i + 1, j);
    }
    
    currentCell->setFreq(futureFreq);
    float a = currentCell->getEnergy() + energyDelta;
    if (aliveBroCount != 0)
        a += (broEnergy / aliveBroCount) * getHarmPower();
    
    currentCell->setEnergy(a);
    //currentCell->setAlive(futureState);
    
    return;
}

Grid::Grid(int width, int height, CellDelegate* cellObserver)
{
    srand(time(0));
    _param = 1.0 - DEFAULT_PARAM;
    _step = 0;
    
    _width = width;
    _height = height;
    
    _cellsGrid = new Cell**[_width];
    for (int i = 0; i < _width; ++i)
    {
        _cellsGrid[i] = new Cell*[_height];
        for (int j = 0; j < _height; ++j)
        {
            _cellsGrid[i][j] = new Cell(this, i, j, false, cellObserver);
        }
    }
}
Grid::~Grid()
{
    for (int i = 0; i < _width; ++i)
    {
        for (int j = 0; j < _height; ++j)
        {
            delete _cellsGrid[i][j];
            _cellsGrid[i][j] = NULL;
        }
        delete [] _cellsGrid[i];
    }
    delete [] _cellsGrid;
}

float Grid::getLifePower()
{
    return 1 -_param;
}

float Grid::getHarmPower()
{
    return _param;
}

void Grid::incParam(float delta)
{
    _param += delta;
}

float Grid::getParam()
{
    return _param;
}

int Grid::getWidth()
{
    return _width;
}

int Grid::getHeight()
{
    return _height;
}

Cell* Grid::getCell(int x, int y)
{
    return _cellsGrid[_cycledIndex(x, _width)][_cycledIndex(y, _height)];
}

Cell* Grid::setCellAlive(int x, int y, bool alive)
{
    _cellsGrid[x][y]->setAlive(alive);
    return _cellsGrid[x][y];
}

void Grid::reset()
{
    for (int i = 0; i < _width; ++i)
        for (int j = 0; j < _height; ++j)
            if (_cellsGrid[i][j])
                _cellsGrid[i][j]->setAlive(false);
}

void Grid::shuffle()
{
    for (int i = 0; i < _width; ++i)
        for (int j = 0; j < _height; ++j)
            if (_cellsGrid[i][j])
            {
                int value = (int)((float)rand() / RAND_MAX + 0.5);
                _cellsGrid[i][j]->setAlive(value == 1);
            }
}

void Grid::advance()
{
    _step++;
    _applyRuleRecursively();
}
