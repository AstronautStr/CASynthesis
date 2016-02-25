//
//  Cell.cpp
//  CAPrototype
//
//  Created by Ilya Solovyov on 22.02.16.
//
//

#include "Cell.h"

CellPresentation::CellPresentation()
{
    mHost = NULL;
    mSelected = false;
    mSelectionAlpha = 0;
}
CellPresentation::CellPresentation(Cell* host)
{
    mHost = host;
    mSelected = false;
    mSelectionAlpha = 0;
}

bool CellPresentation::getSelected()
{
    return mSelected;
}
void CellPresentation::setSelected(bool selected)
{
    mSelected = selected;
}

float CellPresentation::getSelectionAlpha()
{
    return mSelectionAlpha;
}

Color CellPresentation::getColor()
{
    if (mHost != NULL)
    {
        float amp = mHost->getAmp() * 0.1;
        return Color(amp, amp, amp);
    }
    else
        return Color(1.0, 0.0, 0.0);
}

void CellPresentation::update(float dt)
{
    static const float hideTime = .1;
    static const float showTime = .25;
    
    if (mSelected)
        mSelectionAlpha += dt / showTime;
    else
        mSelectionAlpha -= dt / hideTime;
    mSelectionAlpha = ci::math<float>::clamp(mSelectionAlpha);
}

void Cell::init(ivec2 position, double cellsCount, double freq, double amp, ci::audio::NodeRef masterNode)
{
    mPresentation = CellPresentation(this);
    
    osc = ci::audio::master()->makeNode(new ci::audio::GenSineNode);
    gain = ci::audio::master()->makeNode(new ci::audio::GainNode);
    ci::audio::Pan2dNodeRef pan = ci::audio::master()->makeNode(new ci::audio::Pan2dNode);
    pan->setPos((float)rand() / RAND_MAX);
    pan->enable();
    osc->enable();
    osc >> gain >> pan >> masterNode;
    
    mCellsCount = cellsCount;
    
    mGridPosition = position;
    setFreq(freq);
    setAmp(amp);
    setBase(1.0);
    resetNext();
}
Cell::Cell(ivec2 position, double cellsCount, ci::audio::NodeRef masterNode)
{
    init(position, cellsCount, 0, 0, masterNode);
}
Cell::Cell(ivec2 position, double cellsCount, double freq, ci::audio::NodeRef masterNode)
{
    init(position, cellsCount, freq, 1.0, masterNode);
}

CellPresentation& Cell::getPresentation()
{
    return mPresentation;
}

bool Cell::isAlive()
{
    return mAmp > 0.0;
}

double Cell::getAmp()
{
    return mAmp;
}
void Cell::setAmp(double amp)
{
    mAmp = ci::math<double>::clamp(amp);
    
    if (gain != nullptr)
        gain->getParam()->applyRamp(mAmp / mCellsCount, 0.05);
}
void Cell::setNextAmp(double amp)
{
    mNextAmp = amp;
}

void Cell::randFreq(bool next, float lowest, float highest)
{
    double freq = pow(2.0, (log2(lowest) + (log2(highest) - log2(lowest)) * ((float)rand() / RAND_MAX)));
    if (next)
        setNextFreq(freq);
    else
        setFreq(freq);
}

void Cell::applyNext()
{
    setFreq(mNextFreq);
    setAmp(mNextAmp);
}

void Cell::resetNext()
{
    mNextFreq = mFreq;
    mNextAmp = mAmp;
}

double Cell::getFreq()
{
    return mFreq;
}
void Cell::setFreq(double freq)
{
    mFreq = freq;
    
    updateFreq();
}

void Cell::setBase(double base)
{
    mBase = base;
    
    updateFreq();
}

void Cell::updateFreq()
{
    if (osc != nullptr)
    {
        //osc->setFreq(mFreq * mBase);
        osc->getParamFreq()->applyRamp((mFreq * mBase), 0.05);
    }
}

void Cell::setNextFreq(double freq)
{
    mNextFreq = freq;
}

ivec2 Cell::getGridPosition()
{
    return mGridPosition;
}