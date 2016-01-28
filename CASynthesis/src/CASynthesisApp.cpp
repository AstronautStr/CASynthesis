#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CASynthesisApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void CASynthesisApp::setup()
{
}

void CASynthesisApp::mouseDown( MouseEvent event )
{
}

void CASynthesisApp::update()
{
}

void CASynthesisApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( CASynthesisApp, RendererGl )
