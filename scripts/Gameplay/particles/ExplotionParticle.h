#pragma once
#include "./world.h"

namespace Tmpl8
{
	class ExplotionParticle : public Particle
	{
	public:
		ExplotionParticle(float3 _direction, int3 _position, uint _color);


		float3 Position;
		float3 Direction;
	};
}