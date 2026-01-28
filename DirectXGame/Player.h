#pragma once
#include "KamataEngine.h"
#include "Method.h"
#include "TransformWorld.h"

using namespace KamataEngine;

// 前方宣言
class MapChipField;

enum class LRDirection {
	kRight, // 右
	kLeft,  // 左
};

// マップとの当たり判定情報
struct CollisionMapInfo {
	bool onCeilingCollision_ = false; // 天井に当たっているか
	bool onGroundCollision_ = false;  // 地面に当たっているか
	bool onWallCollision_ = false;    // 壁に当たっているか
	bool clampedX_ = false;           // X方向の移動量が制限されたか
	Vector3 moveAmount_;              // 実際に適用する移動量
	int wallNormalX_ = 0;             // 壁の法線（左壁なら+1、右壁なら-1、非接触は0）
};

enum Corner {
	kRightBottom, // 右下
	kLeftBottom,  // 左下
	kRightTop,    // 右上
	kLeftTop,     // 左上
	kNumCorner    // 要素数
};

class Enemy;

class Player {
public:
	void Initialize(Model* model, Camera* camera, const Vector3& position);
	void Update();
	void UpdateFreeze();
	void Draw();
	void Move();

	// ワイヤー開始/解除
	void StartWire(const Vector3& targetWorldPos);
	void CancelWireKeepInertia();
	bool IsWiring() const { return state_ == ActionState::Wire; }

	// 座標・速度まわり
	const KamataEngine::WorldTransform& GetWorldTransform() const;
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; };

	// マップとの当たり判定用
	void SetMapChipField(MapChipField* mapChipField);
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

	// 各種情報取得
	Vector3 GetWorldPosition();
	AABB GetAABB();
	void OnCollision(const Enemy* enemy);
	bool IsDead() const { return isDead_; };
	bool IsOnGround() const { return onGround_; }
	bool IsOnWall() const { return onWall_; }

	// ワイヤー回数ゲッター
	int GetWireLeft() const;

	// ワイヤー使用可能か
	bool CanUseWire() const { return maxWires_ - wiresUsed_ > 0; }

private:
	enum class ActionState { Move, Wire, Dead };
	ActionState state_ = ActionState::Move;

	// ワイヤー設定
	struct WireParams {
		float speed = 0.45f;       // ワイヤー中の移動速度（1/60前提、好みで調整）
		float reachRadius = 0.10f; // ここまで近づいたら到達
		float maxDistance = 12.0f; // 最大距離（GameScene側でもクランプするが保険）
	} wire_;

	Vector3 wireTarget_{};
	bool wireLocked_ = false;

	// 壁ジャンプ処理
	void WallJump(int wallNormalX);

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	Vector3 velocity_ = {};
	LRDirection lrDirection_ = LRDirection::kRight;

	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;

	bool onGround_ = true; // 接地フラグ
	bool onWall_ = false;  // 壁接触フラグ
	MapChipField* mapChipField_ = nullptr;
	bool isDead_ = false;

	// 水平方向移動関連
	static inline const float kAcceleration = 0.03f;
	static inline const float kAttenuation = 0.05f;
	static inline const float kRimitRunSpeed = 0.15f;
	static inline const float kTimeTurn = 0.3f;

	// 重力・落下関連
	static inline const float kGravityAcceleration = 0.02f;
	static inline const float kLimitFallSpeed = 0.3f;

	// 通常ジャンプ初速
	static inline const float kJumpAcceleration = 0.4f;

	// 壁ジャンプ用パラメータ
	static inline const float kWallJumpVerticalSpeed = 0.4f;       // 壁ジャンプの上方向
	static inline const float kWallJumpHorizontalSpeed = 0.5f;     // 壁から離れる横速度
	static inline const float kWallJumpMinDetachVX = 1.0f;         // 最低横速度
	static inline const float kWallJumpMaxHorizontalSpeed = 0.65f; // 壁ジャンプ中の横上限
	static inline const float kWallJumpSeparation = 0.08f;         // 壁から押し出す距離
	static inline const float kWallDetachTime = 0.12f;             // 壁へ戻れない猶予
	static inline const float kWallSlideFallSpeed = 0.08f;         // 壁スライド落下上限

	// プレイヤー当たり判定サイズ
	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;
	static inline const float kBlank = 1.0f;

	// ジャンプ入力バッファ
	float jumpBufferTime_ = 0.10f;
	float jumpBufferLeft_ = 0.0f;

	// コヨーテタイム
	float coyoteTime_ = 0.08f;
	float coyoteLeft_ = 0.0f;

	// ジャンプ回数管理
	int maxJumps_ = 2;
	int jumpsUsed_ = 0;

	// 壁ジャンプ回数（床に触れたときだけ回復）
	int maxWallJumps_ = 1;
	int wallJumpsUsed_ = 0;

	// 壁ジャンプ後のデタッチ
	float wallDetachLeft_ = 0.0f;
	int wallDetachDirX_ = 0; // +1:右へ離脱 / -1:左へ離脱 / 0:なし

	// ワイヤー回数（床に触れたときだけ回復）
	int maxWires_ = 2;
	int wiresUsed_ = 0;

	// ワイヤー開始直後に1フレーム移動しないためのウォームアップ
	int wireWarmupFrames_ = 0;

	// ワイヤー終了フラグ（着地瞬間だけ横速度を整えるため）
	bool wireEndedThisFrame_ = false;
};
