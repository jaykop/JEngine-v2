#include <emitter.hpp>
#include <transform.hpp>
#include <graphic_system.hpp>
#include <shader.hpp>
#include <gl_manager.hpp>
#include <camera.hpp>
#include <asset_manager.hpp>

#include <random.hpp>
#include <math_util.hpp>
#include <mat4.hpp>

#include <thread>

jeBegin

jeDefineComponentBuilder(Emitter);

const unsigned NUM_THREADS = 12;

Emitter::Emitter(Object* owner)
	: Renderer(owner), angle(vec2::zero), velocity(vec3::zero), range(vec3::zero),
	life(1.f), rotationSpeed(0.f), colorSpeed(1.f), pointSize_(0.f), active(true), 
	type(ParticleType::NORMAL), startColor_(vec3::zero), endColor_(vec3::zero), colorDiff_(vec3::zero),
	deadCount_(0), size_(0), drawMode_(GL_TRIANGLES), texture_(0)
{
	sfactor_ = GL_SRC_ALPHA;
	dfactor_ = GL_ONE;
}

Emitter::~Emitter()
{
	remove_from_system();

	for (auto& p : particles_)
	{
		delete p;
		p = nullptr;
	}

	particles_.clear();
}

void Emitter::add_to_system() {

	if (particles_.empty()) {

		for (unsigned i = 0; i < size_; ++i)
			particles_.emplace_back(generate_particle());

		colorDiff_ = (endColor_ - startColor_) / life;
	}

	else
		jeDebugPrint("!Emitter - Already allocated.\n");

	GraphicSystem::add_renderer(this);
}

void Emitter::remove_from_system() {
	GraphicSystem::remove_renderer(this);
}

void Emitter::load(const rapidjson::Value& /*data*/) {

}

void Emitter::draw(float dt)
{
	if (active) {

		Camera* camera = GraphicSystem::get_camera();

		Shader* shader = GLManager::shader_[GLManager::PARTICLE];
		shader->use();

		shader->set_matrix("m4_scale", mat4::scale(transform_->scale));
		shader->set_bool("boolean_bilboard", (status & IS_BILBOARD) == IS_BILBOARD);

		if (prjType == ProjectType::PERSPECTIVE) {

			mat4 perspective = mat4::perspective(
				camera->fovy_ + camera->zoom, camera->aspect_,
				camera->near_, camera->far_);

			shader->set_matrix("m4_projection", perspective);
		}

		else {

			float right_ = GLManager::get_width() * GLManager::resScaler_.x;
			float left_ = -right_;
			float top_ = GLManager::get_height() * GLManager::resScaler_.y;
			float bottom_ = -top_;

			mat4 orthogonal = mat4::orthogonal(left_, right_, bottom_, top_, camera->near_, camera->far_);
			shader->set_matrix("m4_projection", orthogonal);
		}

		// Send camera info to shader
		mat4 viewport = mat4::look_at(camera->position, camera->position + camera->back_, camera->up_);
		shader->set_matrix("m4_viewport", viewport);

		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glBlendFunc(sfactor_, dfactor_);
		glPointSize(pointSize_);

		std::vector< std::thread> threads;
		
		unsigned blockSize = size_ / NUM_THREADS;
		for (unsigned i = 0; i < NUM_THREADS; ++i) {
			if (i != NUM_THREADS - 1)
				threads.emplace_back(std::thread(&Emitter::update_particle, this, 
					std::ref(particles_), dt, i * blockSize, (i + 1) * blockSize));
			else
				threads.emplace_back(std::thread(&Emitter::update_particle, this, 
					std::ref(particles_), dt, i * blockSize, size_));
		}

		for (auto& t : threads)
			t.join();

		glBindVertexArray(GLManager::quadVao_);
		glBindBuffer(GL_ARRAY_BUFFER, GLManager::quadVbo_);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GLManager::quadEbo_);
		glBindTexture(GL_TEXTURE_2D, texture_);

		for (auto& particle : particles_) {

			particle->done = false;
			vec3 viewDirection = (camera->position - particle->position).normalized();

			// Send transform info to shader
			// particle->position.z = transform_->position.z;
			shader->set_matrix("m4_translate", mat4::translate(particle->position));
			shader->set_matrix("m4_rotate", mat4::rotate(Math::deg_to_rad(particle->rotation), viewDirection));

			// Send color info to shader
			shader->set_vec4("v4_color", vec4(particle->color, particle->life));
			shader->set_bool("boolean_hide", particle->hidden);

			glDrawElements(drawMode_, GLManager::quadIndicesSize_, GL_UNSIGNED_INT, nullptr);
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}
}

void Emitter::update_particle(Particles& particles, float dt,
	unsigned start, unsigned end)
{
	for (unsigned i = start; i < end; ++i)
	{
		std::unique_lock<std::mutex> lck(mutex_);
		if (particles[i]->done) continue;
		lck.unlock();

		if (particles[i]->life < 0.f)
			refresh_particle(particles[i]);

		else
		{
			particles[i]->life -= dt;
			particles[i]->position += particles[i]->direction * dt * velocity;

			if (rotationSpeed)
				particles[i]->rotation += particles[i]->rotationSpeed * dt;

			if (colorDiff_ != vec3::zero)
				particles[i]->color += colorDiff_ * dt * colorSpeed;

		}

		lck.lock();
		particles[i]->done = !particles[i]->done ? true : false;
		lck.unlock();
	}
}

Particle* Emitter::generate_particle()
{
	Particle* newParticle = new Particle;

	newParticle->dead = false;
	newParticle->life = Random::get_rand_float(0.f, life);
	newParticle->velocity = Random::get_rand_vec3(-velocity, velocity);
	newParticle->position = transform_->position;
	newParticle->rotation = Random::get_rand_float(0.f, 360.f);
	newParticle->color.set(startColor_);
	newParticle->hidden = type != ParticleType::EXPLOSION;

	if (rotationSpeed)
		newParticle->rotationSpeed = Random::get_rand_float(0.f, rotationSpeed);

	if (angle == vec2::zero)
		newParticle->direction = Random::get_rand_vec3(-vec3::one, vec3::one);

	else
	{
		float rad = Math::deg_to_rad(Random::get_rand_float(angle.x, angle.y));
		newParticle->direction.set(cosf(rad), sinf(rad), 0.f);
	}

	newParticle->direction.normalize();

	return newParticle;
}

void Emitter::refresh_particle(Particle* particle)
{
	particle->life = Random::get_rand_float(0.f, life);
	particle->color.set(startColor_);
	particle->rotation = Random::get_rand_float(0.f, 360.f);
	if (rotationSpeed)
		particle->rotationSpeed = Random::get_rand_float(0.f, rotationSpeed);

	if (type == ParticleType::NORMAL) {

		particle->position = transform_->position;
		particle->hidden = false;
		particle->velocity = Random::get_rand_vec3(-velocity, velocity);
	}

	else if (type == ParticleType::EXPLOSION) {

		// No more particle to update,
		// turn off the active toggle
		if (size_ == deadCount_)
			active = false;

		else if (!particle->dead) {

			// Set dead and add number
			particle->dead = true;
			particle->hidden = true;
			++deadCount_;
		}

		else if (!deadCount_)
		{
			particle->hidden = false;
			particle->dead = false;
			particle->velocity = Random::get_rand_vec3(-velocity, velocity);
			particle->position = transform_->position;
		}

	}

	else if (type == ParticleType::WIDE) {

		particle->hidden = false;

		vec3 position;
		position = transform_->position;
		
		particle->position.x = Random::get_rand_float(position.x - range.x, position.x + range.x);
		particle->position.y = Random::get_rand_float(position.y - range.y, position.y + range.y);
		particle->position.z = Random::get_rand_float(position.z - range.z, position.z + range.z);

		particle->velocity = Random::get_rand_vec3(-velocity, velocity);
		particle->life = Random::get_rand_float(0.f, life);
		particle->color.set(startColor_);
	}

	if (angle == vec2::zero)
		particle->direction = Random::get_rand_vec3(-vec3::one, vec3::one);

	else
	{
		float rad = Math::deg_to_rad(Random::get_rand_float(angle.x, angle.y));
		particle->direction.set(cosf(rad), sinf(rad), 0.f);
	}

	particle->direction.normalize();
	particle->position.z = transform_->position.z;
}

void Emitter::refresh_particles()
{
	deadCount_ = 0;
	for (auto particle : particles_)
		refresh_particle(particle);
}

void Emitter::set_size(unsigned size)
{
	size_ = size;
}

void Emitter::set_colors(const vec3& start, const vec3& end)
{
	startColor_ = start, endColor_ = end;
}

void Emitter::set_texture(unsigned t) { texture_ = t; }

unsigned Emitter::get_texture() const { return texture_; }

void Emitter::set_pointSize(float size)
{
	if (size)
		drawMode_ = GL_POINTS;
	else
		drawMode_ = GL_TRIANGLES;

	pointSize_ = size;
}

float Emitter::get_pointSize() const
{
	return pointSize_;
}

jeEnd