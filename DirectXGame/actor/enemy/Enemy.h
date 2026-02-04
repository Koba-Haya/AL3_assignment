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

	// ★追加：床など外部要因の移動量を加算（次のUpdateで適用）
	void AddExternalMove(const Vector3& delta);

	AABB GetAABB();
	void OnCollision(const Player* player);

	void TakeDamage(int amount);
	void ApplyKnockback(const Vector3& dir, float power);

	bool IsDead() const { return isDead_; }

	// 追加：死亡演出が終わったら true（このタイミングで delete する）
	bool IsDeathEffectFinished() const;

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

	Vector3 externalMove_{0.0f, 0.0f, 0.0f};

	// 死亡演出パラメータ
	static inline const float kDeathFallTimeSec = 0.35f;   // 倒れるまで
	static inline const float kDeathHoldTimeSec = 0.35f;   // 倒れて停止
	static inline const float kDeathShrinkTimeSec = 0.55f; // 縮小して消えるまで
};
