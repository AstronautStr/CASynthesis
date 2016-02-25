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
        return vec4(cell->getEnergy(), cell->getFreq(), 0, alive);
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
    float _time;
    
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
    
    float zoom;
    
public:
    ~CASynthesisApp();
    
	void setup() override;
    
	void mouseDown( MouseEvent event ) override;
    void mouseUp( MouseEvent event ) override;
    void mouseMove( MouseEvent event ) override;
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
    srand(time(0));
    
    _time = timeline().getCurrentTime();
    stepTime = 0.5;

    mousePos = vec2(0, 0);
    zoom = 1;
    
    gridSize = 20;
    const float maxHeight = 600;
    cellSize = maxHeight / gridSize;
    _pause = true;
    
    cellsObserver = new ShaderCellObserver(gridSize, gridSize);
    grid = new Grid(gridSize, gridSize, stepTime, cellsObserver);
    
    glGenBuffers(1, &tbo);
    glGenTextures(1, &tbo_tex);
    
    cinder::BufferRef buffer = PlatformCocoa::get()->loadResource("plain.vert")->getBuffer();
    char* shaderText = (char*)buffer->getData();
    std::string vert(shaderText);
    
    buffer = PlatformCocoa::get()->loadResource("generator.frag")->getBuffer();
    shaderText = (char*)buffer->getData();
    std::string frag(shaderText);
    
    mGlsl = gl::GlslProg::create(gl::GlslProg::Format().vertex(vert).fragment(frag));
    mPlane = gl::Batch::create(geom::Rect(Rectf(-1.0, -1.0, 1.0, 1.0)), mGlsl);
    
    setWindowPos(320, 50);
    setWindowSize(maxHeight, maxHeight);
    
    audio::master()->getOutput()->enableClipDetection(false);
    audio::master()->getOutput()->enable();
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
          
        case KeyEvent::KEY_DOWN:
            grid->incParam(-1.0 / (float)zoom);
            break;
        
        case KeyEvent::KEY_UP:
            grid->incParam(1.0 / (float)zoom);
            break;
        
        case KeyEvent::KEY_RIGHT:
            zoom *= 10;
            break;
            
        case KeyEvent::KEY_LEFT:
            zoom /= 10;
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


void CASynthesisApp::mouseMove( MouseEvent event )
{
    mousePos = event.getPos();
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
    if (event.isLeft())
    {
        modifyCell(event.getPos(), true);
    }
    else if (event.isRight())
    {
        modifyCell(event.getPos(), false);
    }
}

void CASynthesisApp::update()
{
    float newTime = timeline().getCurrentTime();
    float dt = newTime - _time;
    _time = newTime;
    
    if (!_pause)
        updateGrid(dt);
    
    
    hideCursor();
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
    
    
    glBindBuffer(GL_TEXTURE_BUFFER, tbo);
    glBufferData(GL_TEXTURE_BUFFER, cellsObserver->getDataSize(), cellsObserver->getCellsDataRaw(), GL_STATIC_DRAW);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, tbo_tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, tbo);
    glBindSampler(0, 0);
    
    mPlane->draw();
    
    vec2 cellRectSize = vec2((float)getWindowWidth() / gridSize, (float)getWindowHeight() / gridSize);
    
    ivec2 mouseCell = (ivec2)(mousePos / cellRectSize);
    vec2 mouseCellPoint = (vec2)mouseCell * cellRectSize;
    
    gl::enableAdditiveBlending();
    gl::color(1.0, 1.0, 1.0, 0.1 + 0.008 * math<float>::sin(M_PI * _time * 3.0 * (_pause ? 0.0 : 1.0)));
    gl::drawSolidRect(Rectf(((vec2)mouseCell - vec2(1.0, 1.0)) * cellRectSize, ((vec2)mouseCell + vec2(2.0, 2.0)) * cellRectSize));
    gl::disableBlending();
    
    if (grid->getCell(mouseCell.x, mouseCell.y)->isAlive())
    {
        gl::color(1.0, 0.4, 0.05);
    }
    else
    {
        gl::color(0.3, 0.3, 0.3);
    }
    gl::drawSolidRect(Rectf(mouseCellPoint, mouseCellPoint + cellRectSize));
    
    
    ivec2 labelPoint = ivec2(50, 50);
    gl::color(Color(0.0, 0.0, 0.0));
    gl::drawSolidRect(Rectf(labelPoint, (vec2)labelPoint + vec2(50, 10)));
    gl::color(Color(1.0, 1.0, 1.0));
    gl::drawString(to_string(grid->getParam()), labelPoint);
    
}

CINDER_APP( CASynthesisApp, RendererGl )
