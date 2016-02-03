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
        _isAlive = alive;
        
        _delegate = delegate;
        _callDelegate();
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
        return _isAlive;
    }
    
    void setAlive(bool alive)
    {
        _isAlive = alive;
        _callDelegate();
    }
    
    void setDelegate(CellDelegate* delegate)
    {
        _delegate = delegate;
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
    
    void _applyRuleRecursively(int i = 0, int j = 0)
    {
        if (i < 0 || i >= _width || j < 0 || j >=_height)
            return;
        
        Cell* currentCell = _cellsGrid[i][j];
        bool futureState = currentCell->isAlive();
        
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
                    futureState = true;
                }
                else
                {
                    futureState = false;
                }
            }
            else
            {
                if (aliveBroCount == 3)
                {
                    futureState = true;
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

        currentCell->setAlive(futureState);
        return;
    }
    
public:
    Grid(int width, int height, CellDelegate* cellObserver = NULL)
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
