#pragma once
class Camera
{
public:
    Camera();

    void Movement(const float3 direction, const float deltaTime);
    void Zooming(const float scale);

    void SetPositionAndLookat(const float3 newPos, const float3 newLookat);
    void SetPosition(const float3 newPos);
    void SetTargetPos(const float3 newTargetPos);
    void SetLookat(const float3 newLookat);
    void SetSpeed(const float newSpeed);

    float3 GetPos() { return pos; };
    float3 GetLookat() { return lookat; };
    float3 GetCameraUp() { return cameraUp; };

private:
    World* world;
    float3 pos;
    float3 lookat;
    float3 targetPos;
    float moveSpeed;
    float zoomSpeed;
    float3 cameraUp;
    float2 zoomingMinMax;
};