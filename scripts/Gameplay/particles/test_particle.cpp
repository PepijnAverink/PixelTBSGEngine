#include "precomp.h"
#include "./test_particle.h"

namespace Tmpl8
{
	TestParticle::TestParticle(float3 _direction, int3 _position, uint _color)
		: Direction(_direction)
		, Position(make_float3(_position.x, _position.y, _position.z))
	{
		currPos = _position;
		color = _color;
	}
}