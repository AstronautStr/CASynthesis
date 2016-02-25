//
//  Cell.h
//  CAPrototype
//
//  Created by Ilya Solovyov on 22.02.16.
//
//

#ifndef Cell_h
#define Cell_h

#include "cinder/cinder.h"
#include "cinder/audio/audio.h"

using namespace cinder;

class Cell;

class CellPresentation
{
protected:
    bool mSelected;
    float mSelectionAlpha;
    Cell* mHost;
    
    std::string mFreqString;
    std::string mAmpString;
    
public:
    static Color getBorderColor()
    {
        return Color(0.33f, 0.33f, 0.33f);
    }
    static Color getDeadCellColor()
    {
        return Color(0.0f, 0.0f, 0.0f);
    }
    static Color getFreqColor()
    {
        return Color(0.0f, 0.99f, 0.0f);
    }
    static Color getAmpColor()
    {
        return Color(0.99f, 0.0, 0.0f);
    }
    
    CellPresentation();
    CellPresentation(Cell* host);
    
    bool getSelected();
    void setSelected(bool selected);
    
    float getSelectionAlpha();
    
    Color getColor();
    
    void update(float dt);
};

class Cell
{
protected:
    ivec2 mGridPosition;
    double mFreq;
    double mBase;
    double mNextFreq;
    double mNextAmp;
    double mAmp;
    double mCellsCount;
    
    CellPresentation mPresentation;
    
    audio::GenSineNodeRef osc;
    audio::GainNodeRef gain;
    
    void init(ivec2 position, double cellsCount, double freq, double amp);
public:
    Cell(ivec2 position, double cellsCount);
    Cell(ivec2 position, double cellsCount, double freq);
    
    CellPresentation& getPresentation();
    
    bool isAlive();
    
    double getAmp();
    void setAmp(double amp);
    void setNextAmp(double amp);
    
    void randFreq(bool next = false, float lowest = 20, float highest = 20000.0);
    double getFreq();
    void setFreq(double freq);
    void setNextFreq(double freq);
    
    void setBase(double base);
    void updateFreq();
    
    void applyNext();
    void resetNext();
    
    ivec2 getGridPosition();
};

#endif /* Cell_h */
