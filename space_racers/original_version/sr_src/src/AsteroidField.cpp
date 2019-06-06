//source for asteroid field//
#include <Game/AsteroidField.h>
#include <Game/Shaders/Asteroid.h>
#include <Helpers.h>

using namespace Game;

//---------field class---------//

//ctor
AsteroidField::AsteroidField(TextureResource& tr)
	: m_roidCount	(10u),
	m_diffuse		(tr.Get("assets/textures/environment/globe_diffuse.png")),
	m_craterNormal	(tr.Get("assets/textures/environment/crater_normal.jpg")),
	m_globeNormal	(tr.Get("assets/textures/environment/globe_normal.png"))
{
	//load textures
	m_diffuse.setRepeated(true);
	m_craterNormal.setRepeated(true);

	//create shader
	m_roidShader.loadFromMemory(roidMap, sf::Shader::Fragment);
	m_roidShader.setParameter("normalTexture", m_craterNormal);
	m_roidShader.setParameter("globeNormalTexture", m_globeNormal);
	float textureSize = static_cast<float>(m_diffuse.getSize().x);
	m_roidShader.setParameter("texRes", sf::Vector2f(textureSize, textureSize));

	m_shadowShader.loadFromMemory(shadow, sf::Shader::Fragment);

	//set up render state
	m_roidStates.shader = &m_roidShader;
	m_roidStates.texture = &m_diffuse;

	m_shadowStates = m_roidStates;
	m_shadowStates.shader = &m_shadowShader;

	//set vertices to quads
	m_roidVertices.setPrimitiveType(sf::Quads);
	m_shadowVertices.setPrimitiveType(sf::Quads);

	//create field
	const float texSize = static_cast<float>(m_diffuse.getSize().x);
	for(auto i = 0u; i < m_roidCount; ++i)
	{
		const float roidSize = Helpers::Random::Float(texSize / 16.f, texSize / 8.f);
		m_asteroids.push_back(std::unique_ptr<Asteroid>(new Asteroid(roidSize, texSize)));
	}
}


//public
void AsteroidField::Update(float dt,
						   const sf::Vector2f& lightPos,
						   std::vector< std::unique_ptr<Vehicle> >& vehicles,
						   tmx::MapLoader& map)
{
	//update quad tree with field bounds
	map.UpdateQuadTree(m_bounds);

	m_roidVertices.clear();
	m_shadowVertices.clear();
	for(auto&& roid : m_asteroids)
	{
		roid->Update(dt, lightPos);
		//set in different direction if left bounds
		if(!m_bounds.contains(roid->Position))
			roid->InvertVelocity(m_bounds);
		

		//collide with other roids - TODO potemtial quad tree usage
		for(auto&& otherRoid : m_asteroids)
		{
			if(roid != otherRoid)
				//&& bounds.contains(otherRoid->Position))
				roid->Collide(*otherRoid);
		}

		//collide with vehicles
		for(auto&& vehicle : vehicles)
		{
			if(vehicle->GetState() == Alive)
			{
				roid->Collide(*vehicle);
			}
		}

		//collide with world geometry
		std::vector<tmx::MapObject*>& objects = map.QueryQuadTree(roid->GetGlobalBounds());
		//query quadtree
		for(auto&& o : objects)
		{
			//object is solid && contains pos + norm(vel) * rad
			if(o->GetParent() != "Collision") continue;
				
			sf::Vector2f pos = roid->GetPosition();
			sf::Vector2f edge = roid->GetLeadingEdge();
			if(o->Contains(pos + edge)) //TODO better detection than this
			{
				//reflect about normal
				roid->InvertVelocity(o->CollisionNormal(pos, edge));
				break; //skip remaining objects
			}
			else
			{
				roid->InCollision = false;
			}
		}

		roid->Tested = true; //so other roids skip this when testing
		
		//add updated roid vertices for drawing 
		for(const auto& vertex : roid->Vertices)
			m_roidVertices.append(vertex);

		for(const auto& vertex : roid->ShadowVertices)
			m_shadowVertices.append(vertex);
	}

	//reset collsion state
	for(auto&& roid : m_asteroids)
		roid->Tested = false;

	//update shader properties
	m_roidShader.setParameter("lightPos", lightPos);
	//m_shader.setParameter("camPos", viewArea.getCenter());
}

void AsteroidField::SetBounds(const sf::Vector2f& size)
{
	m_bounds = sf::FloatRect(sf::Vector2f(), size);
	
	//randomise roid positions
	for(auto&& roid : m_asteroids)
		roid->SetPosition(sf::Vector2f(Helpers::Random::Float(0.f, size.x), Helpers::Random::Float(0.f, size.y)));

	const float texSize = static_cast<float>(m_diffuse.getSize().x);
	//pad bounds
	m_bounds.width += texSize * 2.f;
	m_bounds.height += texSize * 2.f;
	m_bounds.left -= texSize;
	m_bounds.top -= texSize;
}

//private
void AsteroidField::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(m_shadowVertices, m_shadowStates);	
	target.draw(m_roidVertices, m_roidStates);
}


//---------'roid class---------//

//ctor
AsteroidField::Asteroid::Asteroid(float size, float texSize)
	: Tested	(false),
	InCollision	(false),
	Radius		(size / 2.f),
	m_mass		(2000.f * (size / texSize))
{
	//initialise vertices
	Vertices[1].texCoords.x = texSize;
	Vertices[2].texCoords = sf::Vector2f(texSize, texSize);
	Vertices[3].texCoords.y = texSize;
	SetPosition(Position);

	m_velocity = sf::Vector2f(Helpers::Random::Float(2.f, 10.f), Helpers::Random::Float(2.f, 10.f));
	m_velocity = Helpers::Vectors::Normalize(m_velocity);
	m_velocity *= size * 4.f; //magic number to kill
	m_lastMovement = m_velocity;
}

//public
void AsteroidField::Asteroid::Update(float dt, const sf::Vector2f& lightPos)
{
	//do phys / update logic
	sf::Vector2f translation = m_velocity * dt;
	m_Move(translation);
	m_lastMovement = translation;

	//update vert position for shadows
	sf::Vector2f lightAngle = Position - lightPos;
	if(Helpers::Vectors::GetLength(lightAngle) > 55.f) //TODO kill magic number
	{
		lightAngle = Helpers::Vectors::Normalize(lightAngle) * 55.f;
	}

	for(auto& v : ShadowVertices)
		v.position += lightAngle;
}

void AsteroidField::Asteroid::InvertVelocity(const sf::FloatRect& bounds)
{
	if(Position.x > bounds.left && Position.x < bounds.left + bounds.width)	
		m_velocity.y =- m_velocity.y;
	else 
		m_velocity.x =- m_velocity.x;

	m_Move(-m_lastMovement);
}

void AsteroidField::Asteroid::InvertVelocity(const sf::Vector2f& normal)
{
	if(!InCollision) // only update if not already collided
	{
		//std::cerr << "vx: " << m_velocity.x << ", vy: " << m_velocity.y << " nx: " << normal.x << ", ny: " << normal.y << std::endl;
		m_velocity = Helpers::Vectors::Reflect(m_velocity, normal);
		//std::cerr << "rx: " << m_velocity.x << ", ry: " << m_velocity.y << std::endl;
		m_Move(-m_lastMovement * 2.f);
		InCollision = true;
	}
}

void AsteroidField::Asteroid::Collide(Asteroid& otherRoid)
{
	//check if other roid has been tested against this one already
	if(otherRoid.Tested) return;
	
	sf::Vector2f collision = Position - otherRoid.Position;
	const float length = Helpers::Vectors::GetLength(collision);
	if(length < Radius + otherRoid.Radius)
	{
		collision = Helpers::Vectors::Normalize(collision);

		const float length1 = Helpers::Vectors::Dot(m_velocity, collision);
		const float length2 = Helpers::Vectors::Dot(otherRoid.m_velocity, collision);

		const float p = (2.f * (length1 - length2)) / (m_mass + otherRoid.m_mass);
		if(p < 0) //prevent "clumping"
		{
			m_velocity -= collision * p * otherRoid.m_mass;
			otherRoid.m_velocity += collision * p * m_mass;
		}
		//m_Move(-(m_lastMovement * 2.f));
	}
}

void AsteroidField::Asteroid::Collide(Vehicle& vehicle)
{
	if(vehicle.Invincible()) return;
	sf::Vector2f collision = vehicle.GetCentre() - Position;
	if(Helpers::Vectors::GetLength(collision) < Radius + vehicle.GetRadius())
	{
		collision = Helpers::Vectors::Normalize(collision);

		//check direction of collision vector
		//float diff = Helpers::Vectors::GetAngle(m_velocity) - Helpers::Vectors::GetAngle(vehicle.GetVelocity());
		//if(diff < -90.f || diff > 90.f) collision =- collision;

		const float length1 = Helpers::Vectors::Dot(m_velocity, collision);
		const float length2 = Helpers::Vectors::Dot(vehicle.GetVelocity(), collision);
		const float p = (2.f * (length1 - length2)) / (m_mass + vehicle.GetMass());

		if(p > 0)
		{
			m_velocity -= collision * p * vehicle.GetMass();
			//damage is dealt based on mass and velocity of roid
			vehicle.Impact(collision * p * m_mass, m_mass * (0.14f/* * Helpers::Vectors::GetLength(m_velocity*/));
		}
		//m_Move(-(m_lastMovement * 4.f));
	}
}

void AsteroidField::Asteroid::SetPosition(const sf::Vector2f& position)
{
	Position = position;
	Vertices[0].position.x = Position.x - Radius;
	Vertices[0].position.y = Position.y - Radius;
	Vertices[1].position.x = Position.x + Radius;
	Vertices[1].position.y = Position.y - Radius;
	Vertices[2].position.x = Position.x + Radius;
	Vertices[2].position.y = Position.y + Radius;
	Vertices[3].position.x = Position.x - Radius;
	Vertices[3].position.y = Position.y + Radius;

	//copy to shadow
	ShadowVertices = Vertices;
}

sf::FloatRect AsteroidField::Asteroid::GetGlobalBounds()
{
	return sf::FloatRect(Vertices[0].position.x, Vertices[0].position.y, Radius, Radius);
}

sf::Vector2f AsteroidField::Asteroid::GetPosition()
{
	return Vertices[0].position + (sf::Vector2f(Radius, Radius) / 2.f);
}

sf::Vector2f AsteroidField::Asteroid::GetLeadingEdge()
{
	return Helpers::Vectors::Normalize(m_velocity) * Radius;
}
//private
void AsteroidField::Asteroid::m_Move(const sf::Vector2f& vel)
{
	Position += vel;
	for(auto& vertex : Vertices)
		vertex.position += vel;

	ShadowVertices = Vertices;
}

