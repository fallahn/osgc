///header file for particle for creating skid marks///
#ifndef SKID_PARTICLE_H_
#define SKID_PARTICLE_H_

#include <Game/BaseParticle.h>

namespace Game
{
	class SkidParticle final : public BaseParticle
	{
	public:
		SkidParticle(QuadVerts& verts);

		void Update(float dt);
		static std::shared_ptr<BaseParticle> Create(QuadVerts& verts)
		{return std::shared_ptr<BaseParticle>(new SkidParticle(verts));};
	private:
		float m_transparency;
	};
};

#endif //SKID_PARTICLE_H_