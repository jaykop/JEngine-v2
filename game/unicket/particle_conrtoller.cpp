#include <iostream>
#include "application.hpp"
#include "particle_conrtoller.h"

#include "emitter.hpp"

jeBegin

jeDefineUserComponentBuilder(ExplodingParticleController);

ExplodingParticleController::~ExplodingParticleController()
{
	remove_from_system();
}

void ExplodingParticleController::init() { }

void ExplodingParticleController::update(float /*dt*/)
{
	if (InputHandler::key_triggered(KEY::SPACE))
	{
		std::cout << "explosion!\n";
		Emitter* explosion = get_owner()->get_component<Emitter>();
		explosion->refresh_particles();
		explosion->active = true;
	}

	if (InputHandler::key_pressed(KEY::SPACE))
	{
		std::cout << "space pressed\n";
	}
}

void ExplodingParticleController::close() { }

jeEnd