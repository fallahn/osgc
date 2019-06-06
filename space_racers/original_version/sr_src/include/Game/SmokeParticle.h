///derived class for smoke particles///
#ifndef SMOKE_PARTICLE_H_
#define SMOKE_PARTICLE_H_

#include <Game/BaseParticle.h>

namespace Game
{
	class SmokeParticle final : public BaseParticle
	{
	public:
		SmokeParticle(QuadVerts& verts);

		void Update(float dt);
		static std::shared_ptr<BaseParticle> Create(QuadVerts& verts)
		{return std::shared_ptr<BaseParticle>(new SmokeParticle(verts));};

	private:
		float m_colour, m_radius;
		float m_transparency, m_rotationAmount;
	};
};

#endif // SMOKE_PARTICLE_H_