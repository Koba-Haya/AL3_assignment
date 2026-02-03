#pragma once
#include "KamataEngine.h"
#include "Method.h"
#include "TransformWorld.h"

using namespace KamataEngine;

class Player;
class MapChipField;

class Enemy {
public:
	void Initialize(Model* model, Camera* camera, const Vector3& position);
	void Update();
	void UpdateFreeze();
	void Draw();

	void SetMapChipField(MapChipField* mapChipField);

	AABB GetAABB();
	void OnCollision(const Player* player);

	void TakeDamage(int amount);
	void ApplyKnockback(const Vector3& dir, float power);

	bool IsDead() const { return isDead_; }

private:
	struct CollisionInfo {
		bool onCeilingCollision = false;
		bool onGroundCollision = false;
		bool onWallCollision = false;
		Vector3 moveAmount{};
		int wallNormalX = 0;
	};

	void MoveWithMapCollision_();
	void MapCollisionUp_(CollisionInfo& info);
	void MapCollisionDown_(CollisionInfo& info);
	void MapCollisionRight_(CollisionInfo& info);
	void MapCollisionLeft_(CollisionInfo& info);
	void ApplyCollisionMove_(const CollisionInfo& info);
	void HandleGroundCeiling_(const CollisionInfo& info);
	void HandleWall_(const CollisionInfo& info);
	bool WillFallFromEdge_(float nextMoveX) const;

	void SnapToGround_();

private:
	WorldTransform worldTransform_;
	Model* model_ = nullptr;
	Camera* camera_ = nullptr;

	static inline const float kWalkSpeed = 0.03f;
	Vector3 velocity_{};

	float walkTimer_ = 0.0f;
	static inline const float kWalkMotionAngleStart = -30.0f;
	static inline const float kWalkMotionAngleEnd = 30.0f;
	static inline const float kWalkMotionTime = 2.0f;

	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	AABB aabb_{};

	int hp_ = 2;
	bool isDead_ = false;
	float deathT_ = 0.0f;

	float hurtFlashT_ = 0.0f;

	MapChipField* mapChipField_ = nullptr;
	bool onGround_ = false;

	static inline const float kGravityAcceleration = 0.02f;
	static inline const float kLimitFallSpeed = 0.30f;
};
