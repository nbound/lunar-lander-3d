#pragma once
#include "Shape.h"

// Code adapted from Kevin M. Smith - CS 134 SJSU

class Particle : public Shape {
public:
	Particle();
	glm::vec3 velocity;
	glm::vec3 acceleration;
	glm::vec3 force;

	float angVelocity;
	float angAcceleration;
	float torque;

	float damping;
	float angDamping;

	float mass;

	float lifespan;
	float age;
	float radius;
	float birthtime;

	virtual void update();
	void integrate();
	virtual void draw();
	float getAge();
	void reverseStep(); // undo movement from previous step
};

//  Pure Virtual Function Class - must be subclassed to create new forces.
//
class ParticleForce {
public:
	bool applyOnce = false;
	bool applied = false;
	virtual void updateForce(Particle *) = 0;
};

class ParticleSystem {
public:
	void add(const Particle &);
	void addForce(ParticleForce *);
	void remove(int);
	void reset();
	void update();
	int removeNear(const ofVec3f & point, float dist);
	void draw();
	vector<Particle> particles;
	vector<ParticleForce *> forces;
};

// Some convenient built-in forces

class GravityForce : public ParticleForce {
	ofVec3f gravity;

public:
	GravityForce(const ofVec3f & gravity);
	void set(glm::vec3 g) { gravity = g; }
	void updateForce(Particle *);
};

class TurbulenceForce : public ParticleForce {
	ofVec3f tmin, tmax;

public:
	TurbulenceForce(const ofVec3f & min, const ofVec3f & max);
	void updateForce(Particle *);
};

class ImpulseRadialForce : public ParticleForce {
	float magnitude;

public:
	ImpulseRadialForce(float magnitude);
	void updateForce(Particle *);
};

typedef enum { DirectionalEmitter,
	RadialEmitter,
	SphereEmitter } EmitterType;

//  General purpose Emitter class for emitting sprites
//  This works similar to a Particle emitter
//
class ParticleEmitter : public Shape {
public:
	ParticleEmitter();
	ParticleEmitter(ParticleSystem * s);
	~ParticleEmitter();
	void init();
	void draw();
	void start();
	void spawn(float time);
	void stop();
	void setLifespan(const float life) { lifespan = life; }
	void setVelocity(const ofVec3f & vel) { velocity = vel; }
	void setRate(const float r) { rate = r; }
	void setParticleRadius(const float r) { particleRadius = r; }
	void setEmitterType(EmitterType t) { type = t; }
	void update();
	ParticleSystem * sys;
	float rate; // per sec
	bool oneShot;
	bool fired;
	bool randomLife;
	ofVec3f lifeMinMax;
	ofVec3f velocity;
	float lifespan; // sec
	bool started;
	bool spawning; //determines if emitter spawns new particles while started = true
	float lastSpawned; // ms
	float particleRadius;
	float radius;
	bool visible;
	int groupSize; // number of particles to spawn in a group
	bool createdSys;
	EmitterType type;
};
