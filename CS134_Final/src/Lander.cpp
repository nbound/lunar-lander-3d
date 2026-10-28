#include "Lander.h"

// Custom class extending from Particle class. Represents the lander that the player controls with physics.
Lander::Lander() : Particle(){ // by Nathaniel Fadrigon & Minh Phong Do
	position = glm::vec3(0, 0, 0);

	lifespan = -1; //immortal
	birthtime = 0;
	radius = .5;
	mass = 20;
	model = NULL;
	aglRay = Ray(Vector3(position.x, position.y, position.z), Vector3(0, -1, 0));
	agl = 0.0;

	// directional movement
	thrustForce = glm::vec3(0.0, 0.0, 0.0);
	// rotational movement
	turnTorque = 0.0;

	damping = 0.99;
	angDamping = 0.99;
	alive = true;
	
	// Initialize explosion state
	explosionPending = false;
	explosionTriggered = false;
	explosionTimer = 0.0;
	explosion = NULL;
}

// overrides base class getTransform to rotate around y axis instead of z axis
glm::mat4 Lander::getTransform() {
	glm::mat4 T = glm::translate(glm::mat4(1.0), position); //translation matrix starts as identity matrix, then translated to position
	glm::mat4 R = glm::rotate(glm::mat4(1.0), glm::radians(rotation), glm::vec3(0, -1, 0)); //rotation matrix, 3rd parameter defaults to (0,-1,0) for y-axis rotation
	glm::mat4 S = glm::scale(glm::mat4(1.0), scale); //scaling matrix
	return T * R * S; //combined transformation matrix: scale first, then rotate, then translate (opposite order for multiplication)
}

void Lander::update() { // by Nathaniel Fadrigon & Minh Phong Do
	Particle::update();
	updateModel();
	updateAGLray();
	updateThruster();
	updateExplosion();  // Handle explosion countdown
}

void Lander::updateModel() {  // by Nathaniel Fadrigon
	model->setPosition(position.x, position.y, position.z);
	model->setRotation(0, rotation, 0, -1, 0);
}

void Lander::updateAGLray() {  // by Nathaniel Fadrigon
	aglRay.origin = Vector3(position.x, position.y, position.z);
}

void Lander::initThruster() {  // by Nathaniel Fadrigon & Minh Phong Do
	thruster->type = RadialEmitter;
	thruster->groupSize = 100;
	thruster->velocity = glm::vec3(0.0, -80.0, 0.0);
	thruster->setLifespan(0.2);     // Particles live
	thruster->setParticleRadius(5); // Radius of particles
	thruster->setRate(60);          // num of particles per second
	thruster->spawning = false;
	thruster->start();
}

void Lander::updateThruster() {  // by Nathaniel Fadrigon
	thruster->position = position;
}

void Lander::die() {  // by Nathaniel Fadrigon & Minh Phong Do
	alive = false;

	// Stop all sounds and effects from thruster
	thrusterOff();
    
    // Trigger delayed explosion
    explosionPending = true;
    explosionTimer = 0.3; 
    explosionPosition = position;
}

void Lander::updateExplosion() { // by Minh Phong Do
	if (explosionPending) {
		explosionTimer -= ofGetLastFrameTime();  // Countdown
		explosionPosition = position;  // Track position during countdown
		
		velocity *= 0.92;  // Dampen movement during countdown
		
		if (explosionTimer <= 0.0) {
			// Stop all movement immediately
			velocity = glm::vec3(0, 0, 0);
			force = glm::vec3(0, 0, 0);
			
			// Trigger the explosion
			if (explosion) {
				explosion->sys->particles.clear();  // Clear old particles
				explosion->sys->reset();             // Reset forces
				explosion->position = explosionPosition;
				explosion->start();
			}
			
			// Play explosion sound
			if (!explosionSound.isPlaying()) {
				explosionSound.play();
			}
			
			explosionPending = false;
			explosionTriggered = true;
		}
	}
}

void Lander::thrusterOn() {  // by Nathaniel Fadrigon
	thruster->spawning = true;
	if (!thrusterSound.isPlaying()) {
		thrusterSound.play();
	}
}

void Lander::thrusterOff() {  // by Nathaniel Fadrigon
	thruster->spawning = false;
	if (thrusterSound.isPlaying()) {
		thrusterSound.stop();
	}
}
