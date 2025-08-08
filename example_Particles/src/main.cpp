#include "ofApp.h"

int main(){
  ofGLFWWindowSettings settings;
  settings.setSize(768, 768);
  settings.setGLVersion(4, 1);
  auto mainWindow = ofCreateWindow(settings);
  ofRunApp(new ofApp());
}
