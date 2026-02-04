#pragma once
#include "KamataEngine.h"
#include "TransformWorld.h"

using namespace KamataEngine;

class PlayerBullet {
public:
	void Initialize(Model* model, Camera* camera, const Vector3& pos, const Vector3& vel);
	void Update();
	void Draw();

	bool IsActive() const { return active_; }
	void Deactivate() { active_ = false; }

	AABB GetAABB() const;

private:
	WorldTransform worldTransform_{};
	Model* model_ = nullptr;
	Camera* camera_ = nullptr;

	Vector3 velocity_{};
	bool active_ = false;

	float lifeSec_ = 0.0f;
	static inline const float kLifeTimeSec = 1.6f;
	static inline const float kRadius = 0.18f;
};