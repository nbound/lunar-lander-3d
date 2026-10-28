#include "Particle.h"
#include <cstdlib>
#include <ctime>

// By Nathaniel Fadrigon
// Code adapted from Kevin M. Smith - CS 134 SJSU

Particle::Particle() : Shape(){ //default constructor
	velocity = glm::vec3(0, 0, 0);
	acceleration = glm::vec3(0, 0, 0);
	force = glm::vec3(0, 0, 0);
	angVelocity = 0;
	angAcceleration = 0;
	torque = 0;
	age = 0;
	lifespan = 5;
	birthtime = 0;
	radius = .5;
	damping = 1.0; //no damping by default
	angDamping = 1.0;
	mass = 1;
}

void Particle::update() {
	integrate();
}

void Particle::integrate() {

	float framerate = ofGetFrameRate();
	float dt;
	if (framerate > 0)
		dt = 1.0 / framerate; //avoids div by 0 error in the case of ofGetFrameRate() returning 0
	else
		dt = 1.0 / 60.0;
	position += velocity * dt;
	rotation += angVelocity * dt;

	glm::vec3 accel = acceleration;
	accel += (1.0 / mass * force);
	velocity += accel * dt;

	float angAccel = angAcceleration;
	angAccel += (torque / mass); //angular accel = torque / moment of inertia (approximated here as mass)
	angVelocity += angAccel * dt;

	//dampen values
	velocity *= damping;
	angVelocity *= angDamping;

	force = glm::vec3(0, 0, 0);
	torque = 0.0;
}

void Particle::draw() {
	// ofPushMatrix(); //store current transformation matrix
	// ofMultMatrix(getTransform()); //apply this shape's transformation matrix
	// //draw circle
	// ofSetColor(ofColor::white);
	// ofNoFill();
	// ofDrawSphere(glm::vec3(0, 0, 0), 50.0);
	// ofPopMatrix();
	// ofSetColor(ofMap(getAge(), 0, lifespan, 255, 10));
	ofSetColor(ofColor::blue);     // Pure blue
    ofDrawSphere(position, radius);
}


float Particle::getAge() {
	age = (ofGetElapsedTimeMillis() - birthtime) / 1000.0;
	return age;
}

//------------------------------------------------------------------------------

void ParticleSystem::add(const Particle & p) {
	particles.push_back(p);
}

void ParticleSystem::addForce(ParticleForce * f) {
	forces.push_back(f);
}

void ParticleSystem::remove(int i) {
	particles.erase(particles.begin() + i);
}

void ParticleSystem::reset() {
	for (int i = 0; i < forces.size(); i++) {
		forces[i]->applied = false;
	}
}

void ParticleSystem::update() {
	// check if empty and just return
	if (particles.size() == 0) return;

	vector<Particle>::iterator p = particles.begin();
	vector<Particle>::iterator tmp;

	// check which particles have exceed their lifespan and delete
	// from list.  When deleting multiple objects from a vector while
	// traversing at the same time, we need to use an iterator.
	//
	while (p != particles.end()) {
		if (p->lifespan != -1 && p->getAge() > p->lifespan) {
			tmp = particles.erase(p);
			p = tmp;
		} else
			p++;
	}

	// update forces on all particles first
	//
	for (int i = 0; i < particles.size(); i++) {
		for (int k = 0; k < forces.size(); k++) {
			if (!forces[k]->applied)
				forces[k]->updateForce(&particles[i]);
		}
	}

	// update all forces only applied once to "applied"
	// so they are not applied again.
	//
	for (int i = 0; i < forces.size(); i++) {
		if (forces[i]->applyOnce)
			forces[i]->applied = true;
	}

	// integrate all the particles in the store
	//
	for (int i = 0; i < particles.size(); i++)
		particles[i].update();
}

// remove all particlies within "dist" of point (not implemented as yet)
//
int ParticleSystem::removeNear(const ofVec3f & point, float dist) { return 0; }

//  draw the particle cloud
//
void ParticleSystem::draw() {
	// for (int i = 0; i < particles.size(); i++) {
	// 	particles[i].draw();
	// }
}

//------------------------------------------------------------------------------

GravityForce::GravityForce(const ofVec3f & g) {
	gravity = g;
}

void GravityForce::updateForce(Particle * particle) {
	//
	// f = mg
	//
	particle->force += gravity * particle->mass;
}

//------------------------------------------------------------------------------

TurbulenceForce::TurbulenceForce(const ofVec3f & min, const ofVec3f & max) {
	tmin = min;
	tmax = max;
}

void TurbulenceForce::updateForce(Particle * particle) {
	//
	// We are going to add a little "noise" to a particles
	// forces to achieve a more natual look to the motion
	//
	particle->force.x += ofRandom(tmin.x, tmax.x);
	particle->force.y += ofRandom(tmin.y, tmax.y);
	particle->force.z += ofRandom(tmin.z, tmax.z);
}

//------------------------------------------------------------------------------

// Impulse Radial Force - this is a "one shot" force that
// eminates radially outward in random directions.
//
ImpulseRadialForce::ImpulseRadialForce(float magnitude) {
	this->magnitude = magnitude;
	applyOnce = true;
}

void ImpulseRadialForce::updateForce(Particle * particle) {

	// we basically create a random direction for each particle
	// the force is only added once after it is triggered.
	//
	ofVec3f dir = ofVec3f(ofRandom(-1, 1), ofRandom(-1, 1), ofRandom(-1, 1));
	particle->force += dir.getNormalized() * magnitude;
}

//------------------------------------------------------------------------------

ParticleEmitter::ParticleEmitter() {
	sys = new ParticleSystem();
	createdSys = true;
	init();
}

ParticleEmitter::ParticleEmitter(ParticleSystem * s) {
	if (s == NULL) {
		cout << "fatal error: null particle system passed to ParticleEmitter()" << endl;
		ofExit();
	}
	sys = s;
	createdSys = false;
	if (sys->particles.size() > 0)
		init();
	else { 
		init();
	}
}

ParticleEmitter::~ParticleEmitter() {

	// deallocate particle system if emitter created one internally
	//
	if (createdSys) delete sys;
}

void ParticleEmitter::init() {
	velocity = glm::vec3(1000, 0, 0);
	started = false;
	spawning = true;
	lastSpawned = 0;
	randomLife = false;
	lifeMinMax = ofVec3f(2, 4);
	started = false;
	oneShot = false;
	fired = false;
	radius = 1;
	particleRadius = 1;
	groupSize = 1;
	visible = true;
	type = DirectionalEmitter;
	rate = 1;
	lifespan = 1;
}

void ParticleEmitter::draw() {
	if (visible) {
		switch (type) {
		case DirectionalEmitter:
			ofSetColor(ofColor::white);
			// ofDrawCircle(position, radius);
			break;
		case SphereEmitter:
		case RadialEmitter:
			// ofDrawCircle(position, radius);
			break;
		default:
			break;
		}
	}
	sys->draw();
}
void ParticleEmitter::start() {
	started = true;
	lastSpawned = ofGetElapsedTimeMillis();
}

void ParticleEmitter::stop() {
	started = false;
}

void ParticleEmitter::update() {

	float time = ofGetElapsedTimeMillis();

	if (oneShot && started) {
		if (!fired) {
			for (int i = 0; i < groupSize; i++) spawn(time);
			lastSpawned = time;
		}
		fired = true;
		stop();
	} else if (((time - lastSpawned) > (1000.0 / rate)) && spawning && started) {
		for (int i = 0; i < groupSize; i++) spawn(time);
		lastSpawned = time;
	}
	sys->update();
}

void ParticleEmitter::spawn(float time) {
	Particle particle;
	switch (type) {
		case RadialEmitter: {
			ofVec3f dir = ofVec3f(
            ofRandom(-0.1, 0.1),  // slight X variation
            -1.0,                  // DOWNWARD (negative Y)
            ofRandom(-0.1, 0.1)   // slight Z variation
        );
			float speed = velocity.length();
			particle.velocity = dir.getNormalized() * speed;
			particle.position = position;
			break;
		}
		case SphereEmitter:
			break;
		case DirectionalEmitter:
			particle.velocity = velocity;
			particle.position = position;
			break;
		}
	particle.radius = particleRadius;
	if (randomLife)
		particle.lifespan = ofRandom(lifeMinMax.x, lifeMinMax.y);
	else
		particle.lifespan = lifespan;
	particle.birthtime = time;
	sys->add(particle);
}
