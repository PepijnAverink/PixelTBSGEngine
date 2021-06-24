
#include "precomp.h"
#include "ExplotionParticle.h"


Tmpl8::ExplotionParticle::ExplotionParticle(float3 _direction, int3 _position, uint _color)
	: Direction(_direction)
	, Position(make_float3(_position.x, _position.y, _position.z))
{
	currPos = _position;
	color = _color;
}
