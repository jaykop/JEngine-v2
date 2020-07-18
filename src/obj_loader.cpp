#include <fstream>
#include <sstream>
#include <mesh.hpp>
#include <half_edge_mesh.hpp>
#include <math_util.hpp>
#include <colors.hpp>
#include <asset_manager.hpp>

using namespace Math;

jeBegin

static const float normScale = 0.05f;
static const unsigned max_unsinged = (std::numeric_limits<unsigned>::max)();
//static const vec3 vnormColor = Color::red;
//static const vec3 fnormColor = Color::blue;

AssetManager::MeshMap AssetManager::meshMap_;
vec3 AssetManager::maxPoint, AssetManager::minPoint;

bool AssetManager::load_obj(const char* path)
{
	std::string parsed_key = parse_name(path);

	if (meshMap_.find(parsed_key.c_str()) == meshMap_.end()) {

		std::ifstream obj(path, std::ios::in);
		std::stringstream buffer;
		buffer << obj.rdbuf();

		// Check if obj file is valid 
		if (!obj) return false;

		Mesh* newMesh = new Mesh();
		newMesh->setNormals = true;
		newMesh->key = parsed_key;

		// Initialize checkers
		minPoint.set(max_float), maxPoint.set(min_float);

		parse_vertex(buffer.str(), &newMesh);
		convert_mesh(&newMesh);
		newMesh->hEdgeMesh = new HalfEdgeMesh(newMesh->vertices_, newMesh->indices_);
		calculate_normals(&newMesh);
		Mesh::describe_mesh_attribs(newMesh);

		meshMap_.insert({ parsed_key.c_str(), newMesh });
	}

	// todo: would make some problem
	// key = parsed_key;
	return true;
}

Mesh* AssetManager::get_mesh(const char* name)
{
	auto found = meshMap_.find(name);
	if (found != meshMap_.end())
		return found->second;

	return nullptr;
}

std::string AssetManager::parse_name(const char* name)
{
	std::string res;

	for (int i = 0; name[i] != '\0'; i++) {

		res += name[i];

		if (name[i] == '/')
			res.clear();
	}

	res.erase(res.size() - 4);

	return res;
}

void AssetManager::clear_meshes()
{
	for (auto& m : meshMap_) {
		delete m.second;
		m.second = nullptr;
	}
}

void AssetManager::update_max_min(const vec3& v)
{
	if (v.x < minPoint.x)
		minPoint.x = v.x;
	else
		maxPoint.x = v.x;

	if (v.y < minPoint.y)
		minPoint.y = v.y;
	else
		maxPoint.y = v.y;

	if (v.z < minPoint.z)
		minPoint.z = v.z;
	else
		maxPoint.z = v.z;
}

void AssetManager::convert_mesh(Mesh** mesh)
{
	// Assign the min and max
	(*mesh)->min = minPoint;
	(*mesh)->max = maxPoint;

	vec3 sum = { (*mesh)->max.x - (*mesh)->min.x,
		(*mesh)->max.y - (*mesh)->min.y,
		(*mesh)->max.z - (*mesh)->min.z };

	sum *= .5f;

	if ((*mesh)->absMax < sum.x)
		(*mesh)->absMax = sum.x;
	if ((*mesh)->absMax < sum.y)
		(*mesh)->absMax = sum.y;
	if ((*mesh)->absMax < sum.z)
		(*mesh)->absMax = sum.z;

	(*mesh)->max /= (*mesh)->absMax;
	(*mesh)->min /= (*mesh)->absMax;
	(*mesh)->centerOffset = (((*mesh)->max + (*mesh)->min) * .5f);
	(*mesh)->min -= (*mesh)->centerOffset;
	(*mesh)->max -= (*mesh)->centerOffset;

	unsigned size = unsigned((*mesh)->points_.size());
	vec3 centerOffset = (*mesh)->centerOffset;
	float absMax = (*mesh)->absMax;

	// Set vertex container
	for (unsigned i = 0; i < size; i++) {

		// convert normal
		vec3 convertedPos = get_converted_position((*mesh)->points_[i],
			centerOffset, absMax);

		// Normal vertexes
		(*mesh)->vertices_.push_back(Vertex{ convertedPos, vec3::zero , vec2::zero, vec4::one });
		(*mesh)->vPoints_.push_back(convertedPos);
	}
}

void AssetManager::parse_vertex(const std::string& data, Mesh** mesh)
{
	// skip any leading white space
	unsigned it = unsigned(data.find_first_not_of("\n\r\0 "));
	unsigned size = unsigned((*mesh)->points_.size());

	while (it != max_unsinged)
	{
		// extract vertex data
		if (data[it] == 'v')
			read_vertex(data, it + 1, (*mesh)->points_);

		// extract face data
		if (data[it] == 'f')
			read_face(data, it + 1, (*mesh)->indices_, size);

		// skip to next line
		it = unsigned(data.find_first_of("\n\r\0 ", it));
		it = unsigned(data.find_first_not_of("\n\r\0 ", it));
	}
}

void AssetManager::read_vertex(const std::string& file_data, unsigned pos,
	std::vector<vec3>& points)
{
	vec3 p;
	const char* c_data = file_data.c_str();

	// Read vertex positions
	pos = get_next_elements(file_data, pos);
	p.x = static_cast<float>(atof(c_data + pos));

	pos = get_next_elements(file_data, pos);
	p.y = static_cast<float>(atof(c_data + pos));

	pos = get_next_elements(file_data, pos);
	p.z = static_cast<float>(atof(c_data + pos));

	// Check absolute max value from vertex
	update_max_min(p);

	points.push_back(p);
}

void AssetManager::read_face(const std::string& file_data, unsigned pos,
	std::vector<unsigned>& indice, unsigned vertice_size)
{
	const char* c_data = file_data.c_str();

	// read face indices
	pos = get_next_elements(file_data, pos);
	indice.push_back(read_index(c_data + pos, vertice_size));

	pos = get_next_elements(file_data, pos);
	indice.push_back(read_index(c_data + pos, vertice_size));

	pos = get_next_elements(file_data, pos);
	indice.push_back(read_index(c_data + pos, vertice_size));
}

unsigned AssetManager::read_index(const char* data, unsigned vertice_size)
{
	int index = atoi(data);

	// wrap index if needed
	if (index < 0)
		return unsigned(index + vertice_size);

	return unsigned(index - 1);
}

unsigned AssetManager::get_next_elements(const std::string& file_data, unsigned pos)
{
	// skip past current element
	pos = unsigned(file_data.find_first_of(" ", pos));

	// skip past white space
	return unsigned(file_data.find_first_not_of(" ", pos));
}

void AssetManager::calculate_normals(Mesh** mesh)
{
	std::vector<Vertex>& vertices = (*mesh)->vertices_;
	std::vector<Vertex>& fNormals = (*mesh)->faceNormalsDraw;
	std::vector<Vertex>& vNormals = (*mesh)->vertexNormalsDraw;
	HalfEdgeMesh* h_mesh = (*mesh)->hEdgeMesh;

	const unsigned num_vertices = unsigned(vertices.size());
	for (unsigned i = 0; i < num_vertices; ++i)
	{
		Vertex& vert = vertices[i];

		// find surrounding faces
		auto neighbors = h_mesh->get_neighbor_faces(i);

		for (const Face* f : neighbors)
		{
			// get vertex positions of face
			vec3 p1 = f->edge->vertex->pos;
			vec3 p2 = f->edge->next->vertex->pos;
			vec3 p3 = f->edge->Prev()->vertex->pos;

			// calculate face normal
			vec3 face_normal = vec3::cross(p2 - p1, p3 - p2);

			float len = face_normal.length();
			if (len > 0.0f)
			{
				// add to vertex normal
				vert.normal += face_normal.normalized();
			}
		}

		// re-normalize
		float len = vert.normal.length();
		if (len > 0.0f) {
			vert.normal /= len;

			vNormals.push_back(Vertex{ vertices[i].position, vec3::zero, vec2::zero });
			vNormals.push_back(Vertex{ vertices[i].position + vert.normal * normScale,
				vec3::zero, vec2::zero });
		}
	}

	vec3 centerOffset = (*mesh)->centerOffset;
	float absMax = (*mesh)->absMax;

	auto faces = h_mesh->get_faces();

	for (auto& f : faces)
	{
		// get vertex positions of face
		vec3 p1 = f.edge->vertex->pos;
		vec3 p2 = f.edge->next->vertex->pos;
		vec3 p3 = f.edge->Prev()->vertex->pos;

		// calculate face normal
		vec3 face_normal = vec3::cross(p2 - p1, p3 - p2);
		vec3 center((p1 + p2 + p3) / 3.f);

		vec3 c_center = get_converted_position(center,
			centerOffset, absMax);

		vec3 c_fnorm = get_converted_position(face_normal,
			centerOffset, absMax);

		fNormals.push_back(Vertex{ center, vec3::zero, vec2::zero });
		fNormals.push_back(Vertex{ center + face_normal.normalized() * normScale , 
			vec3::zero, vec2::zero });
	}
}

vec3 AssetManager::get_converted_position(
	const vec3& position, const vec3& centerOffset, float absMax)
{
	return position / absMax - centerOffset;
}

jeEnd