#pragma once
#include "KamataEngine.h"
#include "Method.h"
#include "TransformWorld.h"
#include <algorithm>

using namespace KamataEngine;

class MapChipField;

enum class LRDirection {
	kRight,
	kLeft,
};

struct CollisionMapInfo {
	bool onCeilingCollision_ = false;
	bool onGroundCollision_ = false;
	bool onWallCollision_ = false;
	bool clampedX_ = false;
	Vector3 moveAmount_;
	int wallNormalX_ = 0;
};

enum Corner { kRightBottom, kLeftBottom, kRightTop, kLeftTop, kNumCorner };

class Enemy;

class Player {
public:
	void Initialize(Model* model, Camera* camera, const Vector3& position);
	void Update();
	void UpdateFreeze();
	void Draw();
	void Move();

	void StartWire(const Vector3& targetWorldPos);
	void CancelWireKeepInertia();

	void ToggleWire();

	void AddExternalMove(const Vector3& delta);

	void MapCollisionDetection(CollisionMapInfo& info);
	Vector3 CornerPosition(const Vector3& center, Corner corner);
	void MapCollisionDetectionUp(CollisionMapInfo& info);
	void MapCollisionDetectionDown(CollisionMapInfo& info);
	void MapCollisionDetectionRight(CollisionMapInfo& info);
	void MapCollisionDetectionLeft(CollisionMapInfo& info);
	void ApplyCollisionMove(const CollisionMapInfo& info);
	void HandleCeilingCollision(const CollisionMapInfo& info);
	void HandleGroundCollision(const CollisionMapInfo& info);
	void HandleWallCollision(const CollisionMapInfo& info);

	void OnCollision(const Enemy* enemy);
	bool CanUseWire() const { return maxWires_ - wiresUsed_ > 0; }

	void OnGround() { velocity_.y = 0.0f; }

	// ---- 攻撃 ----
	bool CanAttack() const;
	void BeginDashAttack();
	void UpdateDashAttack();

	// 遠距離
	bool CanShoot() const;
	bool TryConsumeAmmoAndStartReloadIfNeeded();
	void UpdateReload();

	// ------- フラグ 管理 -------
	bool IsWiring() const { return state_ == ActionState::WireShoot || state_ == ActionState::WireHang; }
	bool IsWireAttached() const { return state_ == ActionState::WireHang; }
	bool IsDead() const { return isDead_; };
	bool IsOnGround() const { return onGround_; }
	bool IsOnWall() const { return onWall_; }
	bool IsOnLadder_() const;
	bool IsReloading() const { return reloading_; }
	// 近距離当たり判定（突進中だけ有効）
	bool IsDashAttacking() const { return state_ == ActionState::DashAttack; }

	// ------- セッター -------
	void SetMapChipField(MapChipField* mapChipField);
	void SetPosition(const Vector3& pos) { worldTransform_.translation_ = pos; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

	// ------- ゲッター -------
	Vector3 GetWireVisualTarget() const;

	const KamataEngine::WorldTransform& GetWorldTransform() const;
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; };

	Vector3 GetWorldPosition();
	AABB GetAABB();
	int GetWireLeft() const;
	int GetAmmo() const { return ammo_; }
	int GetAmmoMax() const { return kAmmoMax; }
	// 射撃用：発射位置/方向（弾生成は GameScene でやる）
	Vector3 GetMuzzleWorldPos() const;
	Vector3 GetFacingDir() const;
	AABB GetDashAttackAABB() const;
	float GetReloadRate() const {
		if (!reloading_) {
			return 0.0f;
		}
		return std::clamp(1.0f - (reloadLeft_ / kReloadTimeSec), 0.0f, 1.0f);
	}

private:
	enum class ActionState { Move, WireShoot, WireHang, Ladder, DashAttack, Dead };
	ActionState state_ = ActionState::Move;

	struct WireParams {
		float shootSpeed = 0.8f;
		float maxDistance = 12.0f;
		float shootUpFactor = 0.55f;

		float reelInSpeed = 0.4f;
		float reelInShorten = 4.5f;
		float minLength = 0.8f;

		float swingInputAccel = 0.003f;
		float airDamping = 0.002f;
		float attachInset = 0.02f;
	} wire_{};

	Vector3 wireTip_{};
	Vector3 wireAnchor_{};
	Vector3 wireShootDir_{};
	float wireTraveled_ = 0.0f;

	float wireLength_ = 0.0f;
	float wireReelTargetLength_ = 0.0f;

	void WallJump(int wallNormalX);

	void BeginWireShoot();
	void EndWire();
	void UpdateWireShoot();
	void UpdateWireHang();
	bool TryAttachAtTip(Vector3& outAnchor) const;
	void ApplyRopeConstraint();
	float Dot3(const Vector3& a, const Vector3& b) const;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	Vector3 velocity_ = {};
	LRDirection lrDirection_ = LRDirection::kRight;

	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;

	bool onGround_ = true;
	bool onWall_ = false;
	MapChipField* mapChipField_ = nullptr;
	bool isDead_ = false;

	static inline const float kAcceleration = 0.03f;
	static inline const float kAttenuation = 0.05f;
	static inline const float kRimitRunSpeed = 0.15f;
	static inline const float kTimeTurn = 0.3f;

	static inline const float kGravityAcceleration = 0.02f;
	static inline const float kLimitFallSpeed = 0.3f;

	static inline const float kJumpAcceleration = 0.4f;

	static inline const float kWallJumpVerticalSpeed = 0.4f;
	static inline const float kWallJumpHorizontalSpeed = 0.5f;
	static inline const float kWallJumpMinDetachVX = 1.0f;
	static inline const float kWallJumpMaxHorizontalSpeed = 0.65f;
	static inline const float kWallJumpSeparation = 0.08f;
	static inline const float kWallDetachTime = 0.12f;
	static inline const float kWallSlideFallSpeed = 0.08f;

	static inline const float kLadderClimbSpeed = 0.12f;

	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;
	static inline const float kBlank = 1.0f;

	float jumpBufferTime_ = 0.10f;
	float jumpBufferLeft_ = 0.0f;

	float coyoteTime_ = 0.08f;
	float coyoteLeft_ = 0.0f;

	int maxJumps_ = 2;
	int jumpsUsed_ = 0;

	int maxWallJumps_ = 1;
	int wallJumpsUsed_ = 0;

	float wallDetachLeft_ = 0.0f;
	int wallDetachDirX_ = 0;

	int maxWires_ = 2;
	int wiresUsed_ = 0;

	Vector3 wireTarget_{};
	bool wireLocked_ = false;
	int wireWarmupFrames_ = 0;

	Vector3 externalMove_{0.0f, 0.0f, 0.0f};

	bool wireEndedThisFrame_ = false;

	// ---- 近距離（突進） ----
	float dashAttackLeft_ = 0.0f;
	static inline const float kDashAttackTime = 0.18f;
	static inline const float kDashAttackSpeed = 0.55f;
	static inline const float kDashAttackHitRange = 0.9f;

	// ---- 遠距離（弾数 + リロード）----
	static inline const int kAmmoMax = 6;
	int ammo_ = kAmmoMax;

	bool reloading_ = false;
	float reloadLeft_ = 0.0f;
	static inline const float kReloadTimeSec = 0.9f;

	static inline const float kShotSpeed = 0.85f; // 1フレーム移動量
	static inline const float kShotMuzzleOffsetX = 0.65f;
	static inline const float kShotMuzzleOffsetY = 0.10f;
};
