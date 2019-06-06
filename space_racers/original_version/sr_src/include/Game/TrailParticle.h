///particle system for creating trails behind vehicles///
#ifndef TRAIL_PARTICLE_H_
#define TRAIL_PARTICLE_H_

#include <Game/BaseParticle.h>

namespace Game
{
	class TrailParticle final : public BaseParticle
	{
	public:
		TrailParticle(QuadVerts& verts);

		void Update(float dt);
		static std::shared_ptr<BaseParticle> Create(QuadVerts& verts)
		{return std::shared_ptr<BaseParticle>(new TrailParticle(verts));};

	private:
		float m_transparency;
	};
}

#endif //TRAIL_PARTICLE_H_