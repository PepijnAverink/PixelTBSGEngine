#pragma once
#include "./world.h"

namespace Tmpl8
{
	class TestParticleSystem : public ParticleSystem
	{
	public:
		TestParticleSystem(int3 _position);
		virtual void Update(float dt) override;

		int counter = 0;
	};
}