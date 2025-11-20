#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
  ofEnableAlphaBlending();
  ofSetFrameRate(30);

  fbo.allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGB32F);
  fbo.begin();
  ofClear(ofFloatColor { 0.0, 0.0, 0.0, 1.0});
  fbo.end();
  
  parameters.add(particleSet.getParameterGroup());
  gui.setup(parameters);
}

//--------------------------------------------------------------
void ofApp::update(){
  particleSet.update();
  fbo.begin();
  ofBlendMode(OF_BLENDMODE_ALPHA);
  ofClear(ofFloatColor { 0.0, 0.0, 0.0, 1.0});
  particleSet.draw();
  fbo.end();
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofBlendMode(OF_BLENDMODE_NONE);
  fbo.draw(0, 0);
  if (guiVisible) gui.draw();
  ofSetWindowTitle(ofToString(ofGetFrameRate()) + " / " + ofToString(particleSet.size()));
}

//--------------------------------------------------------------
void ofApp::exit(){
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (key == OF_KEY_TAB) { guiVisible = not guiVisible; return; }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
  for (int i = 0; i < 100; i++) {
    particleSet.add({x, y}, {ofRandom(50.0)-25.0, ofRandom(50.0)-25.0}, ofFloatColor{ofRandom(1.0),ofRandom(1.0),ofRandom(1.0),ofRandom(1.0)}, ofRandom(0.1)-0.05);
  }
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
