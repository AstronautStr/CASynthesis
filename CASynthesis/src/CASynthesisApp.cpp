#include "cinder/app/App.h"
#include "cinder/Timeline.h"

#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/BufferTexture.h"

#include "cinder/audio/audio.h"

#include "cinder/app/cocoa/PlatformCocoa.h"

#include "CellularAutomata.hpp"

#define CORE_SIZE 3

using namespace ci;
using namespace ci::app;
using namespace std;

typedef vec4 ObserverCellState;
class ShaderCellObserver : public CellDelegate
{
protected:
    ObserverCellState* _cellsData;
    int _width;
    int _height;
    
    ObserverCellState _stateFromCellObj(Cell* cell)
    {
        float alive = cell->isAlive() ? 1.0 : 0.0;
        return vec4(0, 0, 0, alive);
    }
    
    virtual void cellStateChanged(Cell* cell)
    {
        int x = cell->getX();
        int y = cell->getY();
        if (x >= 0 && x < _width && y >= 0 && y < _height)
        {
            _cellsData[cell->getX() * _width + cell->getY()] = _stateFromCellObj(cell);
        }
    }
    
public:
    ShaderCellObserver(int width, int height)
    {
        _width = width;
        _height = height;
        
        _cellsData = new ObserverCellState[_width * _height];
    }
    
    ~ShaderCellObserver()
    {
        delete [] _cellsData;
    }

    const ObserverCellState* getCellsData()
    {
        return (ObserverCellState*)getCellsDataRaw();
    }
    
    const void* getCellsDataRaw()
    {
        return _cellsData;
    }
    
    size_t getDataSize()
    {
        return sizeof(ObserverCellState) * _width * _height;
    }
};

class CASynthesisApp : public App
{
private:
    float time;
    
    float gridTime;
    float stepTime;
    
    ShaderCellObserver* cellsObserver;
    Grid* grid;
    int gridSize;
    
    bool _pause;
    float cellSize;
    void updateGrid(float dt);
    
    void modifyCell(vec2 point, bool state);
    
    vec2 mousePos;
    
    ci::gl::GlslProgRef mGlsl;
    ci::gl::BatchRef mPlane;
    
    GLuint tbo;
    GLuint tbo_tex;
    
public:
    ~CASynthesisApp();
    
	void setup() override;
    
	void mouseDown( MouseEvent event ) override;
    void mouseUp( MouseEvent event ) override;
    void mouseDrag( MouseEvent event ) override;
    
    void keyDown( KeyEvent event ) override;
    void keyUp( KeyEvent event ) override;
    
	void update() override;
	void draw() override;
};


CASynthesisApp::~CASynthesisApp()
{
    delete grid;
    delete cellsObserver;
}

void CASynthesisApp::setup()
{
    time = timeline().getCurrentTime();
    stepTime = .1;

    mousePos = vec2(0, 0);
    
    gridSize = 200;
    const float maxHeight = 800;
    cellSize = maxHeight / gridSize;
    _pause = true;
    
    cellsObserver = new ShaderCellObserver(gridSize, gridSize);
    grid = new Grid(gridSize, gridSize, cellsObserver);
    
    cinder::BufferRef buffer = PlatformCocoa::get()->loadResource("plain.vert")->getBuffer();
    char* shaderText = (char*)buffer->getData();
    std::string vert(shaderText);
    
    buffer = PlatformCocoa::get()->loadResource("generator.frag")->getBuffer();
    shaderText = (char*)buffer->getData();
    std::string frag(shaderText);
    
    mGlsl = gl::GlslProg::create(gl::GlslProg::Format().vertex(vert).fragment(frag));
    mPlane = gl::Batch::create(geom::Rect(Rectf(-1.0, -1.0, 1.0, 1.0)), mGlsl);
    
    setWindowSize(maxHeight, maxHeight);
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

void CASynthesisApp::modifyCell(vec2 point, bool state)
{
    ivec2 gridPoint = ivec2(math<int>::clamp(point.x / getWindowWidth() * grid->getWidth(), 0, grid->getWidth() - 1), math<int>::clamp(point.y / getWindowHeight() * grid->getHeight(), 0, grid->getHeight() - 1));
    grid->setCellAlive(gridPoint.x, gridPoint.y, state);
}


void CASynthesisApp::mouseDrag( MouseEvent event )
{
    mousePos = event.getPos();

    if (event.isLeft())
    {
        modifyCell(event.getPos(), true);
    }
    else if (event.isRight())
    {
        modifyCell(event.getPos(), false);
    }
}

void CASynthesisApp::mouseDown( MouseEvent event )
{
}

void CASynthesisApp::mouseUp( MouseEvent event )
{
     modifyCell(event.getPos(), true);
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
    
    mGlsl->uniform("time", time);
    
    mGlsl->uniform("gridWidth", (float)grid->getWidth());
    mGlsl->uniform("gridHeight", (float)grid->getHeight());
    
    mGlsl->uniform("screenWidth", (float)getWindowWidth());
    mGlsl->uniform("screenHeight", (float)getWindowHeight());
    
    mGlsl->uniform("mouseX", mousePos.x);
    mGlsl->uniform("mouseY", mousePos.y);
    
    mGlsl->uniform("gridSampler", 0);
    
    glGenBuffers(1, &tbo);
    glBindBuffer(GL_TEXTURE_BUFFER, tbo);
    
    glBufferData(GL_TEXTURE_BUFFER, cellsObserver->getDataSize(), cellsObserver->getCellsDataRaw(), GL_STATIC_DRAW);
    glGenTextures(1, &tbo_tex);
    
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, tbo_tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, tbo);
    glBindSampler(0, 0);
    
    mPlane->draw();
}

CINDER_APP( CASynthesisApp, RendererGl )
