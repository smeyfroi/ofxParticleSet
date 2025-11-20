#include "ofApp.h"

int main(){
  ofGLWindowSettings settings;
  settings.setSize(1024, 1024);
  settings.setGLVersion(4, 1);
  ofCreateWindow(settings);
  ofRunApp(new ofApp());
}
