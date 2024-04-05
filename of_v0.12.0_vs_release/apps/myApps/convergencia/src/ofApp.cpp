#include "ofApp.h"



//--------------------------------------------------------------
void ofApp::setup() {
	drawUI = true;

	laserWidth = 800;
	laserHeight = 800;
	laserManager.setCanvasSize(laserWidth, laserHeight);

	// if you don't want to manage your own GUI for your
	// app you can add extra params to the laser GUI
	laserManager.addCustomParameter(renderProfileLabel);
	laserManager.addCustomParameter(renderProfileIndex.set("Render Profile", 0, 0, 2));

	laserManager.addCustomParameter(convergence.set("Convergence", 0, 0, 1));
	laserManager.addCustomParameter(amountOfEmissors.set("Emissors", 1, 0, MAX_EMISSORS));
	laserManager.addCustomParameter(timeSpeed.set("Animation speed", 1, 0, 4));

	laserManager.addCustomParameter(colour.set("Colour", ofColor(0, 255, 0), ofColor(0), ofColor(255)));
	laserManager.addCustomParameter(drawAttractors.set("Draw Attractors", false));

	ofParameter<string> description;
	description.setName("description");
	description.set("INSTRUCTIONS : \nTAB to toggle output editor\nSPACE to hide the UI\nC to clear the particles");
	laserManager.addCustomParameter(description);

	for (int i = 0; i < MAX_ATTRACTORS; i++)
	{
		Attractor attractor;
		attractor.x = laserWidth / 2.f;
		attractor.y = laserHeight;
		for (int j = 0; j < MAX_POINTS; j++)
		{
			float x = 0;//tbd
			float y = ofMap(j, 0, MAX_POINTS, laserHeight, 0);
			attractor.points.push_back(ofVec2f(x, y));
		}
		attractors.push_back(attractor);
	}
}

//--------------------------------------------------------------
void ofApp::update() {
	// prepares laser manager to receive new points
	laserManager.update();
}


void ofApp::draw() {
	ofSetWindowTitle(ofToString(ofGetFrameRate()) + " " + ofToString(particles.size()));

	ofBackground(15, 15, 20);

	drawDots();

	// sends points to the DAC
	laserManager.send();

	if(drawUI)
		laserManager.drawUI();


}


void ofApp::drawDots() {

	string renderProfile;
	switch (renderProfileIndex) {
	case 0:
		renderProfile = OFXLASER_PROFILE_DEFAULT;
		break;
	case 1:
		renderProfile = OFXLASER_PROFILE_DETAIL;
		break;
	case 2:
		renderProfile = OFXLASER_PROFILE_FAST;
		break;
	}
	renderProfileLabel = "Render Profile : OFXLASER_PROFILE_" + renderProfile;

	float originX = laserWidth / 2.f;
	float originY = laserHeight;


	float time = (float)ofGetFrameNum() / 100.f;

	//emissors
	for (int i = 0; i < amountOfEmissors; i++)
	{
		Particle particle;
		particle.x = laserWidth / 2;
		particle.y = laserHeight;
		particle.angle = ofRandom(-0.2, 0.2) + PI + HALF_PI;
		particle.attractor = floor(ofRandom(MAX_ATTRACTORS));
		particles.push_back(particle);
	}

	//attractors
	for (int i = 0; i < MAX_ATTRACTORS; i++)
	{
		float convergency = ((float)i / (float)MAX_ATTRACTORS + 1.f) * 10.f;
		float phase = -time * convergency;
		float frequency = (sin(time) + 1.f) / 2.f * 10.f;
		Attractor& attractor = attractors[i];
		for (int j = 0; j < MAX_POINTS; j++)
		{
			float pct = (float)j / (float)MAX_POINTS;
			float radius = (sin(time + pct * PI) + 1.f) / 2.f * 300.f;
			attractor.points[j].x = originX + cos(pct * TWO_PI * frequency + phase) * pct * radius;
		}
	}


	const float MIN_VELOCITY = 1.f * timeSpeed;
	const float MAX_VELOCITY = 10.0f * timeSpeed;

	//*
	// particles
	for (int index = 0; index < particles.size(); ++index) {
		Particle& particle = particles[index];
		particle.angle += (ofNoise(particle.x / 10.f + ofRandom(-1, 1), particle.y / 10.f, time) - 0.46f) * 0.2f;
		float vel = ofMap(particle.y, originY, 0, MIN_VELOCITY, MAX_VELOCITY);
		particle.x += cos(particle.angle) * vel;
		particle.y += sin(particle.angle) * vel;

		if (particle.x < 0 || particle.x > laserWidth || particle.y < 0 || particle.y > laserHeight) {
			particles.erase(particles.begin() + index);
		}
		else {
			ofVec2f attractorPoint = findClosest(particle);
			if (attractorPoint.x != -1 && attractorPoint.y != -1) {
				float angle = findAngle(particle, attractorPoint);
				particle.angle += (custom_mod(angle, TWO_PI) - custom_mod(particle.angle, TWO_PI)) * convergence;
			}
		}
	}

	/**/
	

	/*
	* //TODO
	* Implement TSP to SORT
	* 
	sort(particles.begin(), particles.end(), [](const Particle& p1, const Particle& p2) {
		return p1.x < p2.x;
	});
	sort(particles.begin(), particles.end(), [](const Particle& p1, const Particle& p2) {
		return p1.y < p2.y;
	});
	/**/

	if (drawAttractors) {
		for (auto& attractor : attractors) {
			for (auto& point : attractor.points) {
				laserManager.drawDot(point.x, point.y, ofColor(255, 0, 0), 1, renderProfile);
			}
		}
	}

	for (auto& particle : particles) {
		laserManager.drawDot(particle.x, particle.y, colour, 1, renderProfile);
	}
}


//--------------------------------------------------------------
void ofApp::keyPressed(ofKeyEventArgs& e) {
	if (e.key == OF_KEY_TAB) {
		laserManager.selectNextLaser();
	}
	else if (e.key == ' ') {
		drawUI = !drawUI;
	}
	else if (e.key == 'c') {
		particles.clear();
	}
}

//--------------------------------------------------------------
void ofApp::mouseDragged(ofMouseEventArgs& e) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(ofMouseEventArgs& e) {

}

void ofApp::mouseReleased(ofMouseEventArgs& e) {

}

ofVec2f ofApp::findClosest(Particle& particle) {
	for (auto& point : attractors[particle.attractor].points) {
		if (point.y < particle.y) {
			return point;
		}
	}
	return { -1, -1 };
}

float ofApp::findAngle(Particle& p1, ofVec2f& p2) {
	return atan2(p2.y - p1.y, p2.x - p1.x);
}

float ofApp::custom_mod(float x, float m) {
	return fmod((std::fmod(x, m) + m), m);
}