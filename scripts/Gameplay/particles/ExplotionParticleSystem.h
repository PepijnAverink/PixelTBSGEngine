#pragma once
#include "./world.h"

namespace Tmpl8
{
	class ExplotionParticleSystem : public ParticleSystem
	{
	public:
		ExplotionParticleSystem(int3 _position, float liveTime, int numberOfParticles, uint color, float particleSpeed);
		virtual void Update(float dt) override;

		float speed;
		float timer = 0;
	};
}