/******************************************************************************/
/*!
\file   scene.hpp
\author Jeong Juyong
\par    email: jaykop.jy\@gmail.com
\date   2019/06/02(yy/mm/dd)

\description
Contains the definition of Scene class
*/
/******************************************************************************/

#pragma once
#include <assets.hpp>
#include <vec4.hpp>

jeBegin

// Generic scene class
class Scene {

	// Prevent to clone this class
	Scene() = delete;
	jePreventClone(Scene)

	// Only SceneManager can manage this class
	friend class SceneManager;
	friend class AssetManager;

public:

	const char* get_name() const;
	void register_object(Object* obj);

	// colors
	vec4 background, screen;

protected:

	Scene(const char* name, const char* dir) : name_(name), directory_(dir) {}
	virtual	~Scene() {};

	virtual void load();
	virtual void initialize();
	virtual void update(float dt);
	virtual void close();
	virtual void unload();

	void resume();
	void pause();

	Scene* prevScene_ = nullptr;
	std::string name_, directory_;

	// obj container
	ObjectMap objects_;

	// asset containers
	MeshMap meshes_;
	FontMap fonts_;
	AudioMap audios_;
	TextureMap textures_;
	ArchetypeMap archetypes_;

private:
	
};

jeEnd
