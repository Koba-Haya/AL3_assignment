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
	bool onCeilingCollision_ = false;
	bool onGroundCollision_ = false;
	bool onWallCollision_ = false;
	bool clampedX_ = false;
	Vector3 moveAmount_;
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
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model">モデル</param>
	/// <param name="textureHandle">テクスチャハンドル</param>
	/// <param name="camera">カメラ</param>
	void Initialize(Model* model, Camera* camera, const Vector3& position);

	void Update();

	void UpdateFreeze();

	void Draw();

	void Move();

	void StartAttack();                                                      // ★追加
	bool IsAttacking() const { return state_ == ActionState::AttackActive; } // ★追加

	// 攻撃ヒットボックス公開（Active 中のみ true）
	bool IsAttackHitboxActive() const { return attackHitboxActive_; } // ★追加
	AABB GetAttackAABB() const { return attackAabb_; }                // ★追加

	const KamataEngine::WorldTransform& GetWorldTransform() const;
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; };
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

	// ワールド座標
	Vector3 GetWorldPosition();

	// AABBを取得
	AABB GetAABB();

	// 衝突反応
	void OnCollision(const Enemy* enemy);

	// デスフラグのgetter
	bool IsDead() const { return isDead_; };

	bool IsOnGround() const { return onGround_; } // ← 追加

private:
	// ★修正：攻撃フェーズを分割
	enum class ActionState { Move, AttackWindup, AttackActive, AttackRecovery, Dead };
	ActionState state_ = ActionState::Move;

	// ★追加：攻撃パラメータ
	struct AttackParams {
		// 時間
		float windup = 0.1f;    // ← 0.10s → 0.12s（溜めをわずかに長く）
		float active = 0.16f;   // ← 0.08s → 0.16s（当たり時間を倍に）
		float recovery = 0.18f; // そのまま

		// モーション演出＆判定
		float lungeDistance = 2.2f;

		// ★可変当たり範囲（Active 中に伸びる）
		float rangeMin = 0.8f; // 直前（短い）
		float rangeMax = 1.0f; // 発生終わり（長い）

		// ★可変“幅”（Windup で狭く→Active で戻る）
		float widthMin = 0.8f; // Windup 終端の見た目/判定の“幅”
		float widthMax = 1.0f; // 通常幅（= 既定の幅）
		float height = 0.8f;   // Y 方向は据え置き
	} atk_;

	// ★追加：攻撃制御
	float attackTimer_ = 0.0f; // 現在の攻撃経過時間
	bool attackHitboxActive_ = false;
	AABB attackAabb_{};

	// 攻撃クールタイム
	float attackCooldownSec_ = 0.25f; // 好みで 0.2f〜0.35f くらい
	float attackCooldownLeft_ = 0.0f; // 0以下で発動可能

	// ★追加：攻撃用メソッド
	void UpdateAttack(float dt);
	void BuildAttackAABB(); // 当たり判定の計算
	float EaseOutCubic(float t) const { return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); }

	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;
	// モデル
	KamataEngine::Model* model_ = nullptr;
	// カメラ
	KamataEngine::Camera* camera_ = nullptr;

	Vector3 velocity_ = {};

	LRDirection lrDirection_ = LRDirection::kRight;

	// 旋回開始時の角度
	float turnFirstRotationY_ = 0.0f;
	// 旋回タイマー
	float turnTimer_ = 0.0f;

	// 接地状態フラグ
	bool onGround_ = true;

	// マップチップによるフィールド
	MapChipField* mapChipField_ = nullptr;

	// デスフラグ
	bool isDead_ = false;

	static inline const float kAcceleration = 0.03f;
	static inline const float kAttenuation = 0.05f;
	static inline const float kRimitRunSpeed = 0.15f;
	static inline const float kTimeTurn = 0.3f;

	// 重力加速度（下方向）
	static inline const float kGravityAcceleration = 0.02f;
	// 最大落下速度（下方向）
	static inline const float kLimitFallSpeed = 0.3f;
	// ジャンプ初速（上方向）
	static inline const float kJumpAcceleration = 0.4f;

	// キャラクターの当たり判定サイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	static inline const float kBlank = 1.0f;

	float jumpBufferTime_ = 0.10f; // 入力バッファ猶予
	float jumpBufferLeft_ = 0.0f;

	float coyoteTime_ = 0.08f; // コヨーテ猶予（離陸直後でもジャンプ可）
	float coyoteLeft_ = 0.0f;
};
