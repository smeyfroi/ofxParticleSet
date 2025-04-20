#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
  ofDisableArbTex(); // required for texture2D to work in GLSL, makes texture coords normalized
  ofEnableAlphaBlending();
  ofBackground(ofFloatColor{0.0, 0.0, 0.0, 1.0});

  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>();
  audioDataProcessorPtr = std::make_shared<ofxAudioData::Processor>(audioAnalysisClientPtr);

  fbo.allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGBA32F);
  fbo.begin();
  ofClear(ofFloatColor {0.0, 0.0, 0.0, 0.0});
  fbo.end();

//  motionFromVideo.load(ofToDataPath("trombone-trimmed.mov"), false);
  motionFromVideo.load(ofToDataPath("violin-trimmed.mov"), false);
  
  parameters.add(particleSet.getParameterGroup());
  parameters.add(particleSpin);
  parameters.add(velocityScale);
  parameters.add(potentialNewParticles);
  gui.setup(parameters);
}

//--------------------------------------------------------------
ofColor ofApp::colorAt(float x, float y) {
  float somScale = SOM_WIDTH / motionFromVideo.getMotionFbo().getWidth();
  return somPalette.getColorAt(x, y);
}

void ofApp::addParticles() {
  ofFloatPixels pixels;
  motionFromVideo.getMotionFbo().readToPixels(pixels);
  for (int i = 0; i < potentialNewParticles; i++) {
    float x = ofRandom(pixels.getWidth());
    float y = ofRandom(pixels.getHeight());
    auto c = pixels.getColor(x, y);
    if (c.r > 0.05 || c.r < -0.05 || c.g > 0.05 || c.g < -0.05) {
      particleSet.add({x, y},
                      {c.r * velocityScale, c.g * velocityScale},
                      ofFloatColor{ofRandom(1.0), ofRandom(1.0), ofRandom(1.0), ofRandom(0.3)},
                      particleSpin);
    }
  }
}

void ofApp::update(){
  motionFromVideo.update();

  audioDataProcessorPtr->update();
  float s = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::pitch);
  float t = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare);
  float u = audioDataProcessorPtr->getNormalisedScalarValueMA(ofxAudioAnalysisClient::AnalysisScalar::complexSpectralDifference);
  float v = audioDataProcessorPtr->getNormalisedScalarValueMA(ofxAudioAnalysisClient::AnalysisScalar::spectralCrest);
  float w = audioDataProcessorPtr->getNormalisedScalarValueMA(ofxAudioAnalysisClient::AnalysisScalar::zeroCrossingRate);

  std::vector<ofxAudioData::ValiditySpec> sampleValiditySpecs {
    {ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare, false, 0.01},
    {ofxAudioAnalysisClient::AnalysisScalar::pitch, false, 50},
    {ofxAudioAnalysisClient::AnalysisScalar::pitch, true, 3000}
  };
  
  if (audioDataProcessorPtr->isDataValid(sampleValiditySpecs)) {
    std::array<double, 3> instance {
      static_cast<double>(u),
      static_cast<double>(v),
      static_cast<double>(w)
    };
    somPalette.addInstanceData(instance);
    somPalette.update();
    
//    if (audioDataProcessorPtr->getNormalisedScalarValueMA(ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare) > 0.0) {
    if (t > 0.4) {
      ofLogNotice() << t;
      addParticles();
    }
  }
  
  particleSet.update();

  fbo.begin();
  {
    // fade
    ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
    ofSetColor(ofFloatColor { 0.0, 0.0, 0.0, 0.001});
    ofFill();
    ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());
    // particles
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    particleSet.draw();
  }
  fbo.end();
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, 1.0 });
  fbo.draw(0, 0);
  
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, 0.05 });
  motionFromVideo.getVideoFbo().draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());

  gui.draw();
//  ofSetWindowTitle(ofToString(ofGetFrameRate()) + " / " + ofToString(particleSet.size()));
}

//--------------------------------------------------------------
void ofApp::exit(){
  audioAnalysisClientPtr->closeStream();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (audioAnalysisClientPtr->keyPressed(key)) return;
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

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
