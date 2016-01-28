#include "cinder/app/App.h"
#include "cinder/Timeline.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/audio/audio.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Cell
{
protected:
    ivec2 gridPoint;
    float energy;
    float futureEnergy;
    ci::audio::GenSineNodeRef mSineNode;
    ci::audio::GainNodeRef mGainNode;
    
public:
    Cell(ivec2 gridPoint)
    {
        this->gridPoint = gridPoint;
        energy = 1.0;
        
        auto ctx = audio::Context::master();
        mSineNode = ctx->makeNode(new audio::GenSineNode);
        mGainNode = ctx->makeNode(new audio::GainNode);
        
        mSineNode >> mGainNode >> ctx->getOutput();
        mGainNode->setValue(1.0);
        mSineNode->enable();
    }
    
    const ivec2 getGridPoint()
    {
        return gridPoint;
    }
    
    void setFutureEnergy(float futureEnergy)
    {
        this->futureEnergy = futureEnergy;
    }
    
    void applyFuture()
    {
        this->energy = this->futureEnergy;
        float power = getPower();
        mSineNode->setFreq(power * 440.0);
    }
    
    float getEnergy()
    {
        return energy;
    }
    
    float getPower()
    {
        if (energy == 0)
            return 0.0;
        
        return (pow(2.0, energy) * (energy - fabs(energy)) + (energy + 1) * (fabs(energy) + energy)) / (2 * energy);
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
    
    int** core;
    int coreSize;
    
    Cell* addCell(const ivec2 gridPoint);
    void updateGrid(float dt);
    void nextStepGrid();
    
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
    
    coreSize = 3;
    core = new int*[coreSize];
    for (int i = 0; i < coreSize; ++i)
        core[i] = new int[coreSize];
    
    // core A3
    /*
    core[0][0] = 0;
    core[0][1] = 1;
    core[0][2] = 2;
    
    core[1][0] = -1;
    core[1][1] = 0;
    core[1][2] = 1;
    
    core[2][0] = -2;
    core[2][1] = -1;
    core[2][2] = 0;
    */
    
    // core B3
    
     core[0][0] = 3;
     core[0][1] = 2;
     core[0][2] = 3;
     
     core[1][0] = 2;
     core[1][1] = 0;
     core[1][2] = 2;
     
     core[2][0] = 3;
     core[2][1] = 2;
     core[2][2] = 3;
    
    
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
    
    audio::Context::master()->enable();
}

Cell* CASynthesisApp::addCell(const ivec2 gridPoint)
{
    if (gridPoint.x >= 0 && gridPoint.x < gridSize && gridPoint.y >= 0 && gridPoint.y < gridSize)
    {
        if (grid[gridPoint.x][gridPoint.y] != NULL)
            return NULL;
        
        Cell* newCell = new Cell(gridPoint);
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
                if (bro != NULL)
                    nextStepEnergy += core[radius + i][radius + j] * bro->getEnergy();
            }
        }
        currentCell->setFutureEnergy(nextStepEnergy);
    }
    
    for (std::list<Cell*>::iterator iter = cellsList.begin(); iter != cellsList.end(); ++iter)
        (*iter)->applyFuture();
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
    addCell(ivec2(event.getPos().x / cellSize, event.getPos().y / cellSize));
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
        gl::color((*iter)->getPower() / 5, 0.0, 0.0);
        float x = (*iter)->getGridPoint().x * cellSize;
        float y = (*iter)->getGridPoint().y * cellSize;
        gl::drawSolidRect(Rectf(x, y, x + cellSize, y + cellSize));
        std::ostringstream buff;
        buff << (*iter)->getPower();
        gl::drawString(buff.str(), vec2(x, y));
    }
}

CINDER_APP( CASynthesisApp, RendererGl )
