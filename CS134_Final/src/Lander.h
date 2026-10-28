#pragma once
#include "Particle.h"
#include  "ofxAssimpModelLoader.h"
#include "ray.h"

class Lander : public Particle {
public:
	Lander();
	void die();
	void updateExplosion();  // Handle explosion countdown and triggering
	bool isExploding() { return explosionPending; }
	bool hasExploded() { return explosionTriggered; }
	void updateModel();
	void updateAGLray();
	void initThruster();
	void updateThruster();
	void thrusterOn();
	void thrusterOff();

	ofxAssimpModelLoader * model;
	ParticleEmitter * thruster;
	ParticleEmitter * explosion;  // Reference to explosion emitter
	Ray aglRay;
	float agl;

	// change PWR to modify movement parameters
	// directional movement
	glm::vec3 thrustForce;
	float THRUST_PWR = 200.0;
	// rotational movement
	float turnTorque;
	float TURN_PWR = 2000.0;
	bool alive;

	// Explosion state
	bool explosionPending;
	bool explosionTriggered;
	float explosionTimer;
	glm::vec3 explosionPosition;

	ofSoundPlayer thrusterSound;
	ofSoundPlayer explosionSound;

	// override from shape class
	virtual glm::mat4 getTransform();

	//override from particle class
	virtual void update();
};
