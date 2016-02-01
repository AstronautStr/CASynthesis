#include "cinder/app/App.h"
#include "cinder/Timeline.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/audio/audio.h"

#define CORE_SIZE 3

using namespace ci;
using namespace ci::app;
using namespace std;

class Cell
{
protected:
    ivec2 gridPoint;
    float energy;
    float futureEnergy;
    
    float baseFreq;
    //ci::audio::GenSineNodeRef** mSineNodes;
    ci::audio::GainNodeRef gain;
    ci::audio::GenSineNodeRef** mSineNodes;
    ci::audio::GainNodeRef** mGainNodes;
    float** core;
    
public:
    
    bool isAlive;
    
    Cell(ivec2 gridPoint, ci::audio::NodeRef masterNode)
    {
        this->gridPoint = gridPoint;
        energy = 1.0;
        
        baseFreq = 220.0f;
        
        auto ctx = audio::Context::master();
        
        gain = ctx->makeNode(new audio::GainNode);
        setAlive(false);
        gain >> masterNode;
        
        mSineNodes = new ci::audio::GenSineNodeRef*[CORE_SIZE];
        mGainNodes = new ci::audio::GainNodeRef*[CORE_SIZE];
        core = new float*[CORE_SIZE];
        for (int i = 0; i < CORE_SIZE; ++i)
        {
            mSineNodes[i] = new ci::audio::GenSineNodeRef[CORE_SIZE];
            mGainNodes[i] = new ci::audio::GainNodeRef[CORE_SIZE];
            core[i] = new float[CORE_SIZE];
            for (int j = 0; j < CORE_SIZE; ++j)
            {
                mSineNodes[i][j] = ctx->makeNode(new audio::GenSineNode);
                mGainNodes[i][j] = ctx->makeNode(new audio::GainNode);
                mSineNodes[i][j] >> mGainNodes[i][j] >> gain;
                
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
        
        for (int i = 0; i < CORE_SIZE; ++i)
        {
            for (int j = 0; j < CORE_SIZE; ++j)
            {
                mSineNodes[i][j]->setFreq(baseFreq * core[i][j]);
                mSineNodes[i][j]->enable();
                
                mGainNodes[i][j]->setValue(0.0);
            }
        }
        mGainNodes[1][1]->setValue(1.0);
    }
    
    const ivec2 getGridPoint()
    {
        return gridPoint;
    }
    
    void setFutureEnergy(float futureEnergy)
    {
        //this->futureEnergy = futureEnergy;
        this->energy = futureEnergy;
    }
    
    void applyFuture()
    {
        this->energy = this->futureEnergy;
        float power = getPower();
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
    
    void setAlive(bool alive = true)
    {
        this->isAlive = alive;
        float value = (float)isAlive;
        gain->setValue(value);
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
    
    coreSize = CORE_SIZE;
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
