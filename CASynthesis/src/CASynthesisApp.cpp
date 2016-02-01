#include "cinder/app/App.h"
#include "cinder/Timeline.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/audio/audio.h"

#define CORE_SIZE 3

using namespace ci;
using namespace ci::app;
using namespace std;

class CellularGenNode : public ci::audio::GenNode
{
public:
    CellularGenNode( const Format &format = Format() ) : GenNode( format )	{}
    CellularGenNode( float freq, const Format &format = Format() ) : GenNode( freq, format ) {}
    
protected:
    void process( ci::audio::Buffer *buffer ) override;
};

void CellularGenNode::process( ci::audio::Buffer *buffer )
{
    const auto &frameRange = getProcessFramesRange();
    
    float *data = buffer->getData();
    const float samplePeriod = mSamplePeriod;
    float phase = mPhase;
    
    if( mFreq.eval() )
    {
        const float *freqValues = mFreq.getValueArray();
        for( size_t i = frameRange.first; i < frameRange.second; i++ )
        {
            data[i] = math<float>::sin( phase * float( 2 * M_PI ) );
            phase = fract( phase + freqValues[i] * samplePeriod );
        }
    }
    else
    {
        const float phaseIncr = mFreq.getValue() * samplePeriod;
        for( size_t i = frameRange.first; i < frameRange.second; i++ )
        {
            data[i] = math<float>::sin( phase * float( 2 * M_PI ) );
            phase = fract( phase + phaseIncr );
        }
    }
    
    mPhase = phase;
}

class Cell
{
protected:
    ivec2 gridPoint;
    bool _isAlive;
    float energy;

    float baseFreq;
    float** core;
    
    void _prepareCore()
    {
        core = new float*[CORE_SIZE];
        for (int i = 0; i < CORE_SIZE; ++i)
        {
            core[i] = new float[CORE_SIZE];
            for (int j = 0; j < CORE_SIZE; ++j)
            {
                core[i][j] = 1.0;
            }
        }
        core[0][0] = 9.0;
        core[0][1] = 2.0;
        core[0][2] = 6.0;
        
        core[1][0] = 5.0;
        core[1][1] = 1.0;
        core[1][2] = 3.0;
        
        core[2][0] = 8.0;
        core[2][1] = 4.0;
        core[2][2] = 7.0;
    }
    
public:
    
    Cell(ivec2 gridPoint)
    {
        this->gridPoint = gridPoint;
        _isAlive = false;
        
        energy = 1.0;
        baseFreq = 220.0f;
        
        _prepareCore();
    }
    
    const ivec2 getGridPoint()
    {
        return gridPoint;
    }
    
    float getEnergy()
    {
        return energy;
    }
    
    void setAlive(bool alive = true)
    {
        _isAlive = alive;
    }
    
    bool isAlive()
    {
        return _isAlive;
    }
};

int cycledIndex(int index, int length)
{
    if (index < 0)
        return cycledIndex(length + index, length);
    else if (index >= length)
        return cycledIndex(index - length, length);
    else
        return index;
}

class CASynthesisApp : public App
{
private:
    float time;
    
    float gridTime;
    float stepTime;
    
    int gridSize;
    float cellSize;
    
    std::list<Cell*> cellsList;
    Cell*** grid;
    ci::audio::NodeRef masterNode;
    
    int** core;
    int coreSize;
    
    Cell* addCell(const ivec2 gridPoint);
    void updateGrid(float dt);
    void nextStepGrid();
    
    void aliveCell(const ivec2 gridPoint);
    
public:
	void setup() override;
    
	void mouseDown( MouseEvent event ) override;
    void mouseUp( MouseEvent event ) override;
    
	void update() override;
	void draw() override;
};

void CASynthesisApp::setup()
{
    time = timeline().getCurrentTime();
    stepTime = 1.0;

    gridSize = 20;
    grid = new Cell**[gridSize];
    for (int i = 0; i < gridSize; ++i)
    {
        grid[i] = new Cell*[gridSize];
        for (int j = 0; j < gridSize; ++j)
        {
            grid[i][j] = NULL;
        }
    }
    
    cellSize = 20.0f;
    setWindowSize(gridSize * cellSize, gridSize * cellSize);
    
    auto ctx = audio::Context::master();
    masterNode = ctx->makeNode(new audio::AddNode);
    
    for (int i = 0; i < gridSize; ++i)
        for (int j = 0; j < gridSize; ++j)
            addCell(ivec2(i, j));
    
    masterNode >> ctx->getOutput();
    masterNode->enable();
    ctx->enable();
}

void CASynthesisApp::aliveCell(const ivec2 gridPoint)
{
    if (gridPoint.x >= 0 && gridPoint.x < gridSize && gridPoint.y >= 0 && gridPoint.y < gridSize)
    {
        if (grid[gridPoint.x][gridPoint.y] == NULL)
            return;
        
        grid[gridPoint.x][gridPoint.y]->setAlive();
    }
}

Cell* CASynthesisApp::addCell(const ivec2 gridPoint)
{
    if (gridPoint.x >= 0 && gridPoint.x < gridSize && gridPoint.y >= 0 && gridPoint.y < gridSize)
    {
        if (grid[gridPoint.x][gridPoint.y] != NULL)
            return NULL;
        
        Cell* newCell = new Cell(gridPoint, masterNode);
        grid[gridPoint.x][gridPoint.y] = newCell;
        cellsList.push_back(newCell);
        
        return newCell;
    }
    
    return NULL;
}


void CASynthesisApp::nextStepGrid()
{
    for (std::list<Cell*>::iterator iter = cellsList.begin(); iter != cellsList.end(); ++iter)
    {
        Cell* currentCell = (*iter);
        int radius = coreSize / 2;
        
        float nextStepEnergy = 0.0;
        for (int i = -radius; i <= radius; ++i)
        {
            for (int j = -radius; j <= radius; ++j)
            {
                Cell* bro = grid[cycledIndex(currentCell->getGridPoint().x + i, gridSize)][cycledIndex(currentCell->getGridPoint().y + j, gridSize)];
                if (bro->isAlive)
                    nextStepEnergy += 1.0;
                /*
                    currentCell->mGainNodes[i + radius][j + radius]->setValue(1.0f);
                else
                    currentCell->mGainNodes[i + radius][j + radius]->setValue(0.0f);*/
            }
        }
        currentCell->setFutureEnergy(nextStepEnergy);
    }
    
    /*
     for (std::list<Cell*>::iterator iter = cellsList.begin(); iter != cellsList.end(); ++iter)
        (*iter)->applyFuture();
     */
}

void CASynthesisApp::updateGrid(float dt)
{
    gridTime += dt;
    if (gridTime >= stepTime)
    {
        nextStepGrid();
        gridTime = gridTime - stepTime;
    }
}

void CASynthesisApp::mouseDown( MouseEvent event )
{
}

void CASynthesisApp::mouseUp( MouseEvent event )
{
    ivec2 gridPoint = ivec2(event.getPos().x / cellSize, event.getPos().y / cellSize);
    this->aliveCell(gridPoint);
    //aliveCell(gridPoint);
}

void CASynthesisApp::update()
{
    float newTime = timeline().getCurrentTime();
    float dt = newTime - time;
    time = newTime;
    
    updateGrid(dt);
}

void CASynthesisApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
    
    for (std::list<Cell*>::iterator iter = cellsList.begin(); iter != cellsList.end(); ++iter)
    {
        if ((*iter)->isAlive)
        {
            gl::color(1.0, 0.0, 0.0);
            float x = (*iter)->getGridPoint().x * cellSize;
            float y = (*iter)->getGridPoint().y * cellSize;
            gl::drawSolidRect(Rectf(x, y, x + cellSize, y + cellSize));
            
            std::ostringstream buff;
            buff << (*iter)->getEnergy();
            gl::drawString(buff.str(), vec2(x, y));
        }
    }
}

CINDER_APP( CASynthesisApp, RendererGl )
