//
//  CellularAutomata.hpp
//  CASynthesis
//
//  Created by Ilya Solovyov on 01.02.16.
//
//

#ifndef CellularAutomata_hpp
#define CellularAutomata_hpp

#define DEFAULT_PARAM   0.61803398874989484820

#include <stdio.h>

#include "cinder/audio/audio.h"

float randFreq(float lowest = 20, float highest = 20000.0);
float randLogFreq(float lowest = 20, float highest = 20000.0);
float randFreqCentered(float center, float delta);

class Cell;
class Grid;

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
    
    Grid* _host;
    
    CellDelegate* _delegate;
    
    ci::audio::GenSineNodeRef osc;
    ci::audio::GainNodeRef gain;
    
    void _callDelegate();
    
public:
    Cell(Grid* host, int x, int y, bool alive = false, CellDelegate* delegate = NULL);
    
    int getX();
    int getY();
    
    bool isAlive();
    void setAlive(bool alive);
    
    float getFreq();
    void setFreq(float newFreq);
    
    void setEnergy(float energy);
    float getEnergy();
    
    void setDelegate(CellDelegate* delegate);
};



class Grid
{
    int _width;
    int _height;
    Cell*** _cellsGrid;
    
    unsigned int _step;
    float _param;
    float _generationsTimeStep;
    
protected:
    int _cycledIndex(int index, int length);
    void _applyRuleRecursively(int i = 0, int j = 0);
    
public:
    Grid(int width, int height, float generationsTimeStep, CellDelegate* cellObserver = NULL);
    ~Grid();
    
    float getLifePower();
    float getHarmPower();
    
    void incParam(float delta);
    float getParam();
    
    int getWidth();
    int getHeight();
    
    float getGenerationsTimeStep();
    
    Cell* getCell(int x, int y);
    Cell* setCellAlive(int x, int y, bool alive = true);
    
    void reset();
    void shuffle();
    
    void advance();
};

#endif /* CellularAutomata_hpp */
