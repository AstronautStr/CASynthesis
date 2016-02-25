#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/Timeline.h"
#include "cinder/audio/audio.h"

#include "Cell.h"

using namespace ci;
using namespace ci::app;
using namespace std;

int cycledIndex(int index, int length)
{
    if (index < 0)
        return cycledIndex(length + index, length);
    else if (index >= length)
        return cycledIndex(index - length, length);
    else
        return index;
}

class CAPrototypeApp : public App
{
protected:
    Font mFont;
    vec2 mMousePos;
    float mTime;
    
    double zoom;
    bool mSoundEnabled;
    double mBase;
    double mStepTimer;
    
    int mRuleRadius;
    int mGridSize;
    double mLifePower;
    Cell*** mGrid;
    
    void shuffle();
    void clear();
    void updateBase();
    void modifyCell(ivec2 gridPosition, float amp);
    void applyStepRule();
    
    ivec2 getMouseGridPosition();
    void drawCell(Cell* cell);
    
  public:
    ~CAPrototypeApp();
    
	void setup() override;
    void keyDown( KeyEvent event ) override;
    void mouseMove( MouseEvent event ) override;
    void mouseDrag( MouseEvent event ) override;
    void mouseUp( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void CAPrototypeApp::setup()
{
    srand(time(0));
    mLifePower = 1.0;
    mTime = 0;
    mRuleRadius = 1;
    mGridSize = 8;
    zoom = 1.0;
    
    mBase = 1.0;
    mStepTimer = 0.0;
    
    mSoundEnabled = false;
    
    audio::NodeRef master = audio::master()->getOutput();
    audio::master()->getOutput()->enableClipDetection(false);
    audio::master()->getOutput()->enable();
    
    double cellsCount = mGridSize * mGridSize;
    mGrid = new Cell**[mGridSize];
    for (int i = 0; i < mGridSize; ++i)
    {
        mGrid[i] = new Cell*[mGridSize];
        for (int j = 0; j < mGridSize; ++j)
        {
            mGrid[i][j] = new Cell(ivec2(i, j), cellsCount, master);
            
            mGrid[i][j]->setAmp(0.0);
            mGrid[i][j]->setFreq(0.0);
        }
    }
    
    mFont = Font("Helvetica", 12);
    
    Rectf window = Rectf(vec2(0, 0), vec2(mGridSize, mGridSize));
    Rectf display = Rectf(vec2(0, 0), getDisplay()->getSize());
    float xRatio = display.getWidth() / window.getWidth();
    float yRatio = display.getHeight() / window.getHeight();
    
    float ratio = math<float>::min(xRatio, yRatio);
    float width = window.getWidth() * ratio;
    float height = window.getHeight() * ratio;
    float x = (display.getWidth() - width) / 2;
    float y = (display.getHeight() - height) / 2;
    
    setWindowPos(x, y);
    setWindowSize(width, height);
    

}
CAPrototypeApp::~CAPrototypeApp()
{
    for (int i = 0; i < mGridSize; ++i)
    {
        for (int j = 0; j < mGridSize; ++j)
        {
            delete mGrid[i][j];
        }
        delete mGrid[i];
    }
    delete mGrid;
}

ivec2 CAPrototypeApp::getMouseGridPosition()
{
    vec2 cellDrawSize = getWindowSize() / mGridSize;
    vec2 result = mMousePos / cellDrawSize;
    result.x = cycledIndex(result.x, mGridSize);
    result.y = cycledIndex(result.y, mGridSize);
    
    return result;
}

void CAPrototypeApp::modifyCell(ivec2 gridPosition, float amp)
{
    Cell* cell = mGrid[gridPosition.x][gridPosition.y];
    cell->setAmp(amp);
}

void CAPrototypeApp::shuffle()
{
    for (int i = 0; i < mGridSize; ++i)
    {
        for (int j = 0; j < mGridSize; ++j)
        {
            float life = ((float)rand() / RAND_MAX) > 0.5 ? mLifePower : 0.0;
            mGrid[i][j]->setAmp(life);
            mGrid[i][j]->randFreq();
        }
    }
}
void CAPrototypeApp::clear()
{
    for (int i = 0; i < mGridSize; ++i)
    {
        for (int j = 0; j < mGridSize; ++j)
        {
            mGrid[i][j]->setAmp(0.0);
            mGrid[i][j]->setFreq(0.0);
        }
    }
}
void CAPrototypeApp::updateBase()
{
    for (int i = 0; i < mGridSize; ++i)
    {
        for (int j = 0; j < mGridSize; ++j)
        {
            mGrid[i][j]->setBase(mBase);
        }
    }
}
void CAPrototypeApp::applyStepRule()
{
    for (int i = 0; i < mGridSize; ++i)
    {
        for (int j = 0; j < mGridSize; ++j)
        {
            Cell* cell = mGrid[i][j];
            cell->resetNext();
            
            const double lifePower = mLifePower;
            
            double harmAmp = 0.0;
            double broAmp = 0.0;
            
            // remember that this "- mLifePower" makes cells die each step if it have not bros"
            double nextAmp = cell->getAmp() - mLifePower;
            
            for (int ni = -mRuleRadius; ni <= mRuleRadius; ++ni)
            {
                for (int nj = -mRuleRadius; nj <= mRuleRadius; ++nj)
                {
                    if (ni == 0 && nj == 0)
                        continue;
                    
                    Cell* bro = mGrid[cycledIndex(i + ni, mGridSize)][cycledIndex(j + nj, mGridSize)];
                    broAmp += bro->getAmp();
                    
                    if (bro->getFreq() == 0 || cell->getFreq() == 0)
                        continue;
                    
                    double diff = cinder::math<double>::max(bro->getFreq(), cell->getFreq()) / cinder::math<double>::min(bro->getFreq(), cell->getFreq());
                    
                    const double K = 2;
                    const double l = 0.01;
                    double sum = 0.0;
                    for (int i = 0; i < K - 1; ++i)
                        sum += 1.0 - ci::math<double>::abs(ci::math<double>::sin(ci::math<double>::pow(2.0, i) * M_PI * diff));
                    
                    double dE = (1.0 / K) * (ci::math<double>::floor(K * diff + l) - ci::math<double>::floor(K * diff - l)) * ci::math<double>::abs(ci::math<double>::cos((1.0 / (10.0 * l)) * K * K * M_PI * diff)) * sum;
                    
                    harmAmp += dE * (bro->getAmp() > 0 ? 1.0 : 0.0);
                }
            }
            
            if (!cell->isAlive())
            {
                if (broAmp >= lifePower * 1)
                {
                    nextAmp += lifePower;
                    cell->randFreq(true);
                }
            }
            
            cell->setNextAmp(nextAmp + harmAmp);
        }
    }
    
    for (int i = 0; i < mGridSize; ++i)
        for (int j = 0; j < mGridSize; ++j)
            mGrid[i][j]->applyNext();
}

void CAPrototypeApp::keyDown( KeyEvent event )
{
    ivec2 mouseCell = getMouseGridPosition();
    Cell* cell = mGrid[mouseCell.x][mouseCell.y];
    
    switch (event.getCode())
    {
        case KeyEvent::KEY_SPACE:
            applyStepRule();
            break;
            
        case KeyEvent::KEY_c:
            clear();
            break;
            
        case KeyEvent::KEY_r:
            shuffle();
            break;
            
        case KeyEvent::KEY_DOWN:
            cell->setFreq(cell->getFreq() + -1.0 / zoom);
            break;
            
        case KeyEvent::KEY_UP:
            cell->setFreq(cell->getFreq() + 1.0 / zoom);
            break;
            
        case KeyEvent::KEY_RIGHT:
            zoom *= 10;
            break;
            
        case KeyEvent::KEY_LEFT:
            zoom /= 10;
            break;
            
        case KeyEvent::KEY_q:
            mSoundEnabled = !mSoundEnabled;
            //audio::master()->getOutput()->enable(mSoundEnabled);
            break;
            
        case KeyEvent::KEY_a:
            mBase = 1.0;
            updateBase();
            break;
            
        case KeyEvent::KEY_w:
            mBase = 16.0 / 15.0;
            updateBase();
            break;
            
        case KeyEvent::KEY_s:
            mBase = 9.0 / 8.0;
            updateBase();
            break;
            
        case KeyEvent::KEY_e:
            mBase = 6.0 / 5.0;
            updateBase();
            break;
            
        case KeyEvent::KEY_d:
            mBase = 5.0 / 4.0;
            updateBase();
            break;
            
        case KeyEvent::KEY_f:
            mBase = 4.0 / 3.0;
            updateBase();
            break;
            
        case KeyEvent::KEY_t:
            mBase = 45.0 / 32.0;
            updateBase();
            break;
            
        case KeyEvent::KEY_g:
            mBase = 3.0 / 2.0;
            updateBase();
            break;
            
        case KeyEvent::KEY_y:
            mBase = 8.0 / 5.0;
            updateBase();
            break;
            
        case KeyEvent::KEY_h:
            mBase = 5.0 / 3.0;
            updateBase();
            break;
            
        case KeyEvent::KEY_u:
            mBase = 16.0 / 9.0;
            updateBase();
            break;
            
        case KeyEvent::KEY_j:
            mBase = 15.0 / 8.0;
            updateBase();
            break;
            
        case KeyEvent::KEY_k:
            mBase = 2.0;
            updateBase();
            break;
    }
}

void CAPrototypeApp::mouseMove( MouseEvent event )
{
    mMousePos = event.getPos();
}

void CAPrototypeApp::mouseDrag( MouseEvent event )
{
    mMousePos = event.getPos();
    
    if (event.isLeft())
    {
        modifyCell(getMouseGridPosition(), mLifePower);
    }
    else if (event.isRight())
    {
        modifyCell(getMouseGridPosition(), 0.0);
    }
}

void CAPrototypeApp::mouseUp( MouseEvent event )
{
    if (event.isLeft())
    {
        modifyCell(getMouseGridPosition(), mLifePower);
    }
    else if (event.isRight())
    {
        modifyCell(getMouseGridPosition(), 0.0);
    }
}

void CAPrototypeApp::update()
{
    float currentTime = timeline().getCurrentTime();
    float dt = currentTime - mTime;
    mTime = currentTime;
    
    ivec2 mouseCell = getMouseGridPosition();
    
    for (int i = 0; i < mGridSize; ++i)
    {
        for (int j = 0; j < mGridSize; ++j)
        {
            Cell* cell = mGrid[i][j];
            
            bool selection = (mouseCell.x == cell->getGridPosition().x) && (mouseCell.y == cell->getGridPosition().y);
            cell->getPresentation().setSelected(selection);
            cell->getPresentation().update(dt);
        }
    }
    
    mStepTimer += dt;
    if (mStepTimer >= 0.05)
    {
        mStepTimer = 0.05;
    
        if (mSoundEnabled)
        {
            applyStepRule();
            mStepTimer = 0;
        }
    }
}

void CAPrototypeApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
    
    for (int i = 0; i < mGridSize; ++i)
    {
        for (int j = 0; j < mGridSize; ++j)
        {
            drawCell(mGrid[i][j]);
        }
    }
}

void CAPrototypeApp::drawCell(Cell* cell)
{
    vec2 cellDrawSize = getWindowSize() / mGridSize;
    vec2 cellDrawPos = (vec2)cell->getGridPosition() * cellDrawSize;
    
    gl::color(CellPresentation::getBorderColor());
    gl::drawSolidRect(Rectf(cellDrawPos, cellDrawPos + cellDrawSize));
    
    gl::color(cell->getPresentation().getColor());
    gl::drawSolidRect(Rectf(cellDrawPos + vec2(0.5, 0.5), cellDrawPos + cellDrawSize - vec2(0.5, 0.5)));

    float stringAlpha = cell->isAlive() ? 1.0 : 0.15;
    gl::drawString(toString(cell->getAmp()), cellDrawPos + vec2(cellDrawSize.y / 4, cellDrawSize.y / 4), ColorA(CellPresentation::getAmpColor(), stringAlpha), mFont);
    gl::drawString(toString(cell->getFreq()), cellDrawPos + vec2(cellDrawSize.y / 4, 2 * cellDrawSize.y / 4), ColorA(CellPresentation::getFreqColor(), stringAlpha), mFont);
    
    gl::color(CellPresentation::getBorderColor());
    gl::drawSolidRect(Rectf(vec2(cellDrawPos.x + cellDrawSize.x - cellDrawSize.y / 4, cellDrawPos.y), vec2(cellDrawPos.x + cellDrawSize.x, cellDrawPos.y + cellDrawSize.y)));
    
    gl::color(cell->getPresentation().getColor());
    gl::drawSolidRect(Rectf(vec2(cellDrawPos.x + cellDrawSize.x - cellDrawSize.y / 4, cellDrawPos.y + 0.5), vec2(cellDrawPos.x + cellDrawSize.x - 0.5, cellDrawPos.y + cellDrawSize.y - 0.5)));
}

CINDER_APP( CAPrototypeApp, RendererGl )
