#pragma once
#include "./world.h"

namespace Tmpl8
{
	class TestParticle : public Particle
	{
	public:
		TestParticle(float3 _direction, int3 _position, uint _color);


		float3 Position;
		float3 Direction;
	};
}