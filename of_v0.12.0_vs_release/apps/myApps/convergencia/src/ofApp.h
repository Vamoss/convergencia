#pragma once

#include "ofMain.h"
#include "ofxLaserManager.h"

#define MAX_EMISSORS 20
//100
#define MAX_ATTRACTORS 5
//20
#define MAX_POINTS 100
//1000

struct Attractor {
	float x;
	float y;
	vector<ofVec2f> points;
};

struct Particle {
	ofVec2f pos;
	float angle;
	int attractor;
};

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();

	void keyPressed(ofKeyEventArgs& e);
	void mouseDragged(ofMouseEventArgs& e);
	void mousePressed(ofMouseEventArgs& e);
	void mouseReleased(ofMouseEventArgs& e);

	void drawDots();

	ofParameter<float> convergence;
	ofParameter<int> amountOfEmissors;
	ofParameter<float> timeSpeed;
	ofParameter<int> renderProfileIndex;
	ofParameter<string> renderProfileLabel;
	ofParameter<bool> drawAttractors;
	ofParameter<bool> useNearestNeighbor;

	ofxLaser::Manager laserManager;

	int laserWidth;
	int laserHeight;

	bool drawUI;

	std::vector<ofPolyline> polyLines;

	ofParameter<ofColor> colour;

	vector<Attractor> attractors;
	vector<Particle> particles;

	ofVec2f findClosest(Particle& particle);
	float findAngle(Particle& p1, ofVec2f& p2);
	float custom_mod(float x, float m);
	vector<int> nearestNeighbor();
};

