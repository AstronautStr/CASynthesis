//
//  CellularAutomata.hpp
//  CASynthesis
//
//  Created by Ilya Solovyov on 01.02.16.
//
//

#ifndef CellularAutomata_hpp
#define CellularAutomata_hpp

#include <stdio.h>

class Cell
{
    bool _isAlive;
    bool _futureAlive;
    int _x;
    int _y;
    
public:
    Cell(int x, int y, bool isAlive = false)
    {
        _x = x;
        _y = y;
        _isAlive = _futureAlive = false;
    }
    
    bool isAlive()
    {
        return _isAlive;
    }
    
    void setAlive(bool alive)
    {
        _isAlive = alive;
        _futureAlive = alive;
    }
    
    void setFutureAlive(bool alive)
    {
        _futureAlive = alive;
    }
    
    void applyFuture()
    {
        _isAlive = _futureAlive;
    }
};

class Grid
{
    int _width;
    int _height;
    Cell*** _cellsGrid;
    
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
    
    void _applyRule()
    {
        for (int i = 0; i < _width; ++i)
        {
            for (int j = 0; j < _height; ++j)
            {
                Cell* currentCell = _cellsGrid[i][j];
                if (currentCell != NULL)
                {
                    const int radius = 1;
                    unsigned int aliveBroCount = 0;
                    
                    for (int nx = -radius; nx <= radius; ++nx)
                    {
                        for (int ny = -radius; ny <= radius; ++ny)
                        {
                            if (_cellsGrid[_cycledIndex(i + nx, _width)][_cycledIndex(j + ny, _height)]->isAlive())
                                aliveBroCount++;
                        }
                    }
                    
                    if (currentCell->isAlive())
                    {
                        if (aliveBroCount == 2 || aliveBroCount == 3)
                        {
                            currentCell->setFutureAlive(true);
                        }
                        else
                        {
                            currentCell->setFutureAlive(false);
                        }
                    }
                    else
                    {
                        if (aliveBroCount == 3)
                        {
                            currentCell->setFutureAlive(true);
                        }
                    }
                }
            }
        }
        
        for (int i = 0; i < _width; ++i)
            for (int j = 0; j < _height; ++j)
                _cellsGrid[i][j]->applyFuture();
    }
    
public:
    Grid(int width, int height)
    {
        srand(time(0));
        _step = 0;
        
        _width = width;
        _height = height;
        
        _cellsGrid = new Cell**[_width];
        for (int i = 0; i < _width; ++i)
        {
            _cellsGrid[i] = new Cell*[_height];
            for (int j = 0; j < _height; ++j)
            {
                _cellsGrid[i][j] = new Cell(i, j);
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
                _cellsGrid[i][j] = nullptr;
            }
            delete [] _cellsGrid[i];
        }
        delete [] _cellsGrid;
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
                    int value = (int)((float)rand() / RAND_MAX + 0.5);
                    _cellsGrid[i][j]->setAlive(value == 0);
                }
    }
    
    void advance()
    {
        _step++;
        _applyRule();
    }
};

#endif /* CellularAutomata_hpp */
