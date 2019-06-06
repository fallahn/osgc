///creates multi coloured stars for round win///
#ifndef STAR_PARTICLE_H_
#define STAR_PARTICLE_H_

#include <Game/BaseParticle.h>
#include <Helpers.h>

namespace Game
{
	class StarParticle final : public BaseParticle
	{
	public:
		StarParticle(QuadVerts& verts);

		void Update(float dt);
		static std::shared_ptr<BaseParticle> Create(QuadVerts& verts)
		{return std::shared_ptr<BaseParticle>(new StarParticle(verts));};

	private:
		float m_colour, m_scale;
		sf::Uint8 m_transparency;
	};
};

#endif //STARPARTICLE