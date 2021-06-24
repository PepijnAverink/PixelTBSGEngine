#include "precomp.h"
#include "./test_particle_system.h"
#include "./test_particle.h"


namespace Tmpl8
{
	TestParticleSystem::TestParticleSystem(int3 _position)
	{
		for (uint32_t i = 0; i < 100; i++)
		{
			float r0 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			float r1 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			float r2 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			float3 direction = make_float3(r0, r1, r2);

			TestParticle* p = new TestParticle(direction, _position, rand()%255);
			Particle.push_back(p);
		}
	}

	void TestParticleSystem::Update(float dt)
	{
		counter++;
		for (uint32_t i = 0; i < Particle.size(); i++)
		{
			TestParticle* p = (TestParticle*)Particle[i];
			p->Position += p->Direction;

			Particle[i]->currPos = make_int3(p->Position.x, p->Position.y, p->Position.z);
		}

		if (counter > 100)
			Active = false;
	}
}