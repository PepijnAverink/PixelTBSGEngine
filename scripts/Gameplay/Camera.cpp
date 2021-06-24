#include "precomp.h"
#include "Camera.h"

Camera::Camera()
{
    world = GetWorld();
    pos = make_float3(0, 0, 0);
    lookat = make_float3(0, 0, 0);
    moveSpeed = 0.1f;
    zoomSpeed = 10;
    targetPos = make_float3(0, 0, 0);
    cameraUp = make_float3(0, 1, 0);
    zoomingMinMax = make_float2(30, 400);
}

void Camera::Movement(const float3 direction, const float deltaTime)
{
    float3 cameraViewDir = world->GetCameraViewDir();
    float angle = atan2(cameraViewDir.z, cameraViewDir.x);
    float2 dir = RotateVector2(make_float2(direction.x, direction.z), angle);

    SetPosition(pos + (make_float3(dir.x,direction.y, dir.y) * moveSpeed * deltaTime));
}

void Camera::Zooming(const float scale)
{
    float3 direction = normalize(pos - lookat) * scale;
    float3 newPos = pos + (direction * zoomSpeed);
    if (newPos.y >= zoomingMinMax.x && newPos.y <= zoomingMinMax.y)
    {
        SetPosition(newPos);
    }
}

void Camera::SetPositionAndLookat(const float3 newPos, const float3 newLookat)
{
    pos = newPos;
    lookat = newLookat;
    LookAt(pos, lookat);
}

void Camera::SetPosition(const float3 newPos)
{
    float3 diff = newPos - pos;
    lookat += diff;
    pos = newPos;
    LookAt(pos, lookat);
}

void Camera::SetTargetPos(const float3 newTargetPos)
{
    targetPos = newTargetPos;
}

void Camera::SetLookat(const float3 newLookat)
{
    lookat = newLookat;
    LookAt(pos, lookat);
}

void Camera::SetSpeed(const float newSpeed)
{
    moveSpeed = newSpeed;
}
