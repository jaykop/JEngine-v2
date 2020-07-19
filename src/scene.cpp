/******************************************************************************/
/*!
\file   application.cpp
\author Jeong Juyong
\par    email: jaykop.jy\@gmail.com
\date   2019/06/02(yy/mm/dd)

\description
Contains the methods of scene class
*/
/******************************************************************************/

#include <scene.hpp>
#include <object.hpp>
#include <object_manager.hpp>
#include <asset_manager.hpp>

#include <sound_system.hpp>
#include <physics_system.hpp>
#include <graphic_system.hpp>
#include <behavior_system.hpp>

#include <text.hpp>

jeBegin

void Scene::load()
{
	// Read asset info
	JsonParser::read_file(directory_);
	const rapidjson::Value& textures = JsonParser::get_document()["Texture"];

	// Read font info
	const rapidjson::Value& fonts = JsonParser::get_document()["Font"];

	// Load textures 
	for (rapidjson::SizeType i = 0; i < textures.Size(); ++i) {
		AssetManager::load_texture(textures[i]["Directory"].GetString(), textures[i]["Key"].GetString(),
			&textures_);
		jeDebugPrint("*AssetManager - Loaded image: %s.\n", textures[i]["Directory"].GetString());
	}

	// Load font
	for (rapidjson::SizeType i = 0; i < fonts.Size(); ++i) {

		// Load default ascii characters (0 - 128)
		AssetManager::load_font(fonts[i]["Directory"].GetString(),
			fonts[i]["Key"].GetString(),
			fonts[i]["Size"].GetUint(),
			0, 128, &fonts_);

		// Load additional korean set
		for (unsigned j = 0; j < fonts[i]["Additional"].Size(); ++j) {
			AssetManager::load_font(fonts[i]["Directory"].GetString(),
				fonts[i]["Key"].GetString(),
				fonts[i]["Size"].GetUint(),
				static_cast<unsigned long>(fonts[i]["Additional"][j][0].GetUint64()),
				static_cast<unsigned long>(fonts[i]["Additional"][j][1].GetUint64()),
				&fonts_);
		}
	}

	// bind the objects to the manager
	ObjectManager::objects_ = &objects_;
}

void Scene::initialize()
{
	BehaviorSystem::initialize();
	GraphicSystem::initialize();
	PhysicsSystem::initialize();
	SoundSystem::initialize();
}

void Scene::update(float dt)
{
	// update all systems
	BehaviorSystem::update(dt);
	SoundSystem::update(dt);
	PhysicsSystem::update(dt);
	GraphicSystem::update(dt);
}

void Scene::close()
{
	BehaviorSystem::close();
	SoundSystem::close();
	PhysicsSystem::close();
	GraphicSystem::close();
}

void Scene::unload()
{	
	// unload all assets for current scene
	for (auto& font : fonts_)
	{
		delete font.second;
		font.second = nullptr;
	}

	fonts_.clear();
	textures_.clear();
	audios_.clear();
	archetypes_.clear();

	// make sure current object map belongs to the current scene
	ObjectManager::objects_ = &objects_;

	// clear current object map
	ObjectManager::clear_objects();

	// initialize
	ObjectManager::objects_ = nullptr;
}

const char* Scene::get_name() const
{
	return name_;
}

void Scene::register_object(Object* obj) {

	// check if the object is pointing null
	if (ObjectManager::objects_) {
		ObjectManager::objects_->insert({ obj->get_name(), obj });
		obj->register_components();
	}
}

jeEnd