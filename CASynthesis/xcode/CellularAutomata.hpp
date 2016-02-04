//
//  CellularAutomata.hpp
//  CASynthesis
//
//  Created by Ilya Solovyov on 01.02.16.
//
//

#ifndef CellularAutomata_hpp
#define CellularAutomata_hpp

#define LIFE 1.0f

#include <stdio.h>

float randFreq(float lowest = 20, float highest = 20000.0);
float randFreqCentered(float center, float delta);

class Cell;

class CellDelegate
{
    friend class Cell;
    
protected:
    virtual void cellStateChanged(Cell* cell) = 0;
};

class Cell
{
    bool _isAlive;
    int _x;
    int _y;
    
    float _energy;
    float _freq;
    
    CellDelegate* _delegate;
    
    void _callDelegate()
    {
        if (_delegate != NULL)
            _delegate->cellStateChanged(this);
    }
    
public:
    Cell(int x, int y, bool alive = false, CellDelegate* delegate = NULL)
    {
        _x = x;
        _y = y;
        
        _freq = randFreq();
        _delegate = delegate;
        
        setAlive(alive);
    }
    
    int getX()
    {
        return _x;
    }
    
    int getY()
    {
        return _y;
    }
    
    bool isAlive()
    {
        //return _isAlive;
        return _energy > 0;
    }
    
    void setAlive(bool alive)
    {
        /*if (_isAlive != alive)
            _energy = alive ? 1.0 : 0.0;
        
        _isAlive = alive;*/
        setEnergy(alive ? 1.0 : 0.0);
    }
    
    float getFreq()
    {
        return _freq;
    }
    
    void setEnergy(float energy)
    {
        _energy = energy;
        _callDelegate();
    }
    
    float getEnergy()
    {
        return _energy;
    }
    
    void setDelegate(CellDelegate* delegate)
    {
        _delegate = delegate;
    }
    
    void setFreq(float newFreq)
    {
        _freq = newFreq;
    }
};



class Grid
{
    int _width;
    int _height;
    Cell*** _cellsGrid;
    float _param;
    
    unsigned int _step;
    
protected:
    int _cycledIndex(int index, int length)
    {
        if (index < 0)
            return _cycledIndex(length + index, length);
        else if (index >= length)
            return _cycledIndex(index - length, length);
        else
            return index;
    }
    
    void _applyRuleRecursively(int i = 0, int j = 0)
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
                    if (bro->isAlive())
                    {
                        aliveBroCount++;
                        
                        
                        float diff = cinder::math<float>::max(bro->getFreq(), currentCell->getFreq()) / cinder::math<float>::min(bro->getFreq(), currentCell->getFreq());
                        
                        float dE = cinder::math<float>::clamp(bro->getEnergy() / currentCell->getEnergy()) * (diff - (int)diff);
                        broEnergy += dE;
                        
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
                    energyDelta -= LIFE;
                }
            }
            else
            {
                if (aliveBroCount == 3)
                {
                    //futureState = true;
                    energyDelta += LIFE;
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
        currentCell->setEnergy(currentCell->getEnergy() + energyDelta + (broEnergy / aliveBroCount) * _param);
        //currentCell->setAlive(futureState);
        
        return;
    }
    
public:
    Grid(int width, int height, CellDelegate* cellObserver = NULL)
    {
        srand(time(0));
        _param = 0;//0.00037;
        _step = 0;
        
        _width = width;
        _height = height;
        
        _cellsGrid = new Cell**[_width];
        for (int i = 0; i < _width; ++i)
        {
            _cellsGrid[i] = new Cell*[_height];
            for (int j = 0; j < _height; ++j)
            {
                _cellsGrid[i][j] = new Cell(i, j, false, cellObserver);
            }
        }
    }
    ~Grid()
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
    
    void incParam(float delta)
    {
        _param += delta;
    }
    
    float getParam()
    {
        return _param;
    }
    
    int getWidth()
    {
        return _width;
    }
    
    int getHeight()
    {
        return _height;
    }
    
    Cell* getCell(int x, int y)
    {
        return _cellsGrid[_cycledIndex(x, _width)][_cycledIndex(y, _height)];
    }
    
    Cell* setCellAlive(int x, int y, bool alive = true)
    {
        _cellsGrid[x][y]->setAlive(alive);
        return _cellsGrid[x][y];
    }
    
    void reset()
    {
        for (int i = 0; i < _width; ++i)
            for (int j = 0; j < _height; ++j)
                if (_cellsGrid[i][j])
                    _cellsGrid[i][j]->setAlive(false);
    }
    
    void shuffle()
    {
        for (int i = 0; i < _width; ++i)
            for (int j = 0; j < _height; ++j)
                if (_cellsGrid[i][j])
                {
                    int value = (int)((float)rand() / RAND_MAX + 0.2);
                    _cellsGrid[i][j]->setAlive(value == 1);
                }
    }
    
    void advance()
    {
        _step++;
        _applyRuleRecursively();
    }
};

#endif /* CellularAutomata_hpp */
