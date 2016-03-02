//
//  Cell.cpp
//  CAPrototype
//
//  Created by Ilya Solovyov on 22.02.16.
//
//

#include "Cell.h"
#include "Defines.h"

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
        float amp = 0.5 * (mHost->getAmp() > 0 ? 1.0 : 0.0);
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
    
    ci::audio::Pan2dNodeRef pan = ci::audio::master()->makeNode(new ci::audio::Pan2dNode);
    pan->setPos((float)rand() / RAND_MAX);
    pan->enable();
    
    oscSize = 2;
    _oscArray = new ci::audio::GenSineNodeRef[oscSize];
    _gainArray = new ci::audio::GainNodeRef[oscSize];
    for (int i = 0; i < oscSize; ++i)
    {
        osc = _oscArray[i] = ci::audio::master()->makeNode(new ci::audio::GenSineNode);
        gain = _gainArray[i] = ci::audio::master()->makeNode(new ci::audio::GainNode);
        
        osc->enable();
        osc >> gain >> pan >> masterNode;
    }
    
    oscCounter = 0;
    updateActiveOsc();
    
    mCellsCount = cellsCount;
    
    mGridPosition = position;
    setFreq(freq, false);
    setBase(1.0);
    setAmp(amp, false);
    
    mState = rand() % 16;
    
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
Cell::~Cell()
{
    delete [] _oscArray;
    delete [] _gainArray;
}

void Cell::updateActiveOsc()
{
    osc = _oscArray[oscCounter % oscSize];
    gain = _gainArray[oscCounter % oscSize];
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
void Cell::setAmp(double amp, bool fade)
{
    mAmp = ci::math<double>::clamp(amp);
    
    setGainValue(mAmp, fade);
}
void Cell::setGainValue(double gainValue, bool fade, bool reset)
{
    if (gain != nullptr)
    {
        if (reset)
            gain->getParam()->setValue(0.0);
        
        if (fade)
            gain->getParam()->applyRamp(gainValue / mCellsCount, ATTACK_TIME);
        else
            gain->setValue(gainValue / mCellsCount);
    }
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
void Cell::setFreq(double freq, bool crossfade)
{
    crossfade = crossfade && (mFreq != freq);
    mFreq = freq;
    
    updateFreq(crossfade);
}

void Cell::setBase(double base)
{
    if (base == mBase)
        return;
    
    mBase = base;
    
    updateFreq(false);
}

void Cell::updateFreq(bool swapOsc)
{
    if (osc != nullptr)
    {
        if (swapOsc)
        {
            setGainValue(0.0);
            oscCounter++;
            updateActiveOsc();
            setGainValue(1.0, true, true);
        }
        osc->getParamFreq()->setValue(mBase * mFreq);
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