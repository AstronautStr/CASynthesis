#include "cinder/app/App.h"
#include "cinder/Timeline.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/audio/audio.h"

#include "CellularAutomata.hpp"

#define CORE_SIZE 3

using namespace ci;
using namespace ci::app;
using namespace std;


class CASynthesisApp : public App
{
private:
    float time;
    
    float gridTime;
    float stepTime;
    
    Grid* grid;
    int gridSize;
    
    bool _pause;
    float cellSize;
    void updateGrid(float dt);
    
    vec2 mousePos;
    
public:
    ~CASynthesisApp();
    
	void setup() override;
    
	void mouseDown( MouseEvent event ) override;
    void mouseUp( MouseEvent event ) override;
    void mouseMove( MouseEvent event ) override;
    
    void keyDown( KeyEvent event ) override;
    void keyUp( KeyEvent event ) override;
    
	void update() override;
	void draw() override;
};


CASynthesisApp::~CASynthesisApp()
{
    delete grid;
}

void CASynthesisApp::setup()
{
    time = timeline().getCurrentTime();
    stepTime = 1.0;

    mousePos = vec2(0, 0);
    
    gridSize = 30;
    cellSize = 20.0;
    _pause = true;
    
    grid = new Grid(gridSize, gridSize);

    setWindowSize(grid->getWidth() * cellSize, grid->getHeight() * cellSize);
}


void CASynthesisApp::updateGrid(float dt)
{
    gridTime += dt;
    if (gridTime >= stepTime)
    {
        grid->advance();
        gridTime = gridTime - stepTime;
    }
}


void CASynthesisApp::keyDown( KeyEvent event )
{
    switch (event.getCode())
    {
        case KeyEvent::KEY_SPACE:
            _pause = !_pause;
            break;
            
        case KeyEvent::KEY_r:
            grid->reset();
            grid->shuffle();
            break;
            
        case KeyEvent::KEY_c:
            grid->reset();
            break;
    }
}

void CASynthesisApp::keyUp( KeyEvent event )
{
    
}

void CASynthesisApp::mouseMove( MouseEvent event )
{
    mousePos = event.getPos();
}

void CASynthesisApp::mouseDown( MouseEvent event )
{
}

void CASynthesisApp::mouseUp( MouseEvent event )
{
    ivec2 gridPoint = ivec2(math<int>::clamp(event.getPos().x / cellSize, 0, grid->getWidth() - 1), math<int>::clamp(event.getPos().y / cellSize, 0, grid->getHeight() - 1));
    
    grid->setCellAlive(gridPoint.x, gridPoint.y);
}

void CASynthesisApp::update()
{
    float newTime = timeline().getCurrentTime();
    float dt = newTime - time;
    time = newTime;
    
    if (!_pause)
        updateGrid(dt);
}

void CASynthesisApp::draw()
{
    Color clearColor = Color(0, 0, 0);
    if (_pause)
        clearColor = Color(0, 0.05, 0.15);
        
    gl::clear(clearColor);
    
    for (int i = 0; i < grid->getWidth(); ++i)
    {
        for (int j = 0; j < grid->getHeight(); ++j)
        {
            Cell* currentCell = grid->getCell(i, j);
            
            float x = i * cellSize;
            float y = j * cellSize;
            
            gl::color(clearColor);
            if (currentCell->isAlive())
            {
                gl::color(1.0, 0.0, 0.0);
            }
            else if ((int)mousePos.x / (int)cellSize == i && (int)mousePos.y / (int)cellSize == j)
            {
                gl::color(1.0, 1.0, 0.0);
            }
            gl::drawSolidRect(Rectf(x, y, x + cellSize, y + cellSize));
        }
    }
}

CINDER_APP( CASynthesisApp, RendererGl )
