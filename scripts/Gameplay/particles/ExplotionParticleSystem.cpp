#include "precomp.h"
#include "./ExplotionParticle.h"
#include "./ExplotionParticleSystem.h"


namespace Tmpl8
{
	ExplotionParticleSystem::ExplotionParticleSystem(int3 _position, float liveTime, int numberOfParticles, uint color, float particleSpeed)
	{
		for (uint32_t i = 0; i < numberOfParticles; i++)
		{
			float r0 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			float r1 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			float r2 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			float3 direction = make_float3(r0, abs(r1), r2);
			ExplotionParticle* p = new ExplotionParticle(direction, _position, color);
			Particle.push_back(p);
		}
		timer = liveTime;
		speed = particleSpeed;
	}

	void ExplotionParticleSystem::Update(float dt)
	{
		timer -= dt;
		for (uint32_t i = 0; i < Particle.size(); i++)
		{
			ExplotionParticle* p = (ExplotionParticle*)Particle[i];
			p->Position += p->Direction * dt * speed;

			Particle[i]->currPos = make_int3(p->Position.x, p->Position.y, p->Position.z);
		}

		if (timer <= 0)
		{
			Active = false;
		}
	}
}